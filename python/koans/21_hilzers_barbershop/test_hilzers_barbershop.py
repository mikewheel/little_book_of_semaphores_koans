import threading
import time

from koan_utils import (
    EventLog,
    OverlapTracker,
    ThreadRunner,
    assert_completes,
    eventually,
    jitter,
)

from hilzers_barbershop import HilzersBarbershop

# Test parameters: a shrunken shop (capacity 8, sofa 3, 2 barbers) so the
# suite stays fast; the book's shop is 20/4/3.
CAPACITY, SOFA, BARBERS = 8, 3, 2


class Hooks:
    """Duck-typed hook object; every hook defaults to a no-op."""

    def __init__(self, **overrides):
        for name in (
            "enter_shop",
            "sit_on_sofa",
            "sit_in_chair",
            "pay",
            "cut_hair",
            "accept_payment",
        ):
            setattr(self, name, overrides.get(name, lambda _id: None))


def spawn_customers(runner, shop, count, results, results_lock, stagger=0.0):
    def visit(cid):
        if stagger:
            time.sleep(stagger * cid)
        jitter()
        served = shop.customer_visit(cid)
        with results_lock:
            results[cid] = served

    for cid in range(count):
        runner.spawn(visit, cid, name=f"customer-{cid}")


def test_capacity_respected():
    tracker = OverlapTracker()

    def enter_shop(cid):
        snapshot = tracker.enter("in_shop")
        if snapshot["in_shop"] > CAPACITY:
            tracker.violate(
                f"{snapshot['in_shop']} customers inside a shop of "
                f"capacity {CAPACITY}"
            )

    def pay(cid):
        # Slight under-measurement (the customer stays until the receipt),
        # but exit-at-pay can never overcount — see README contract.
        time.sleep(0.002)
        tracker.exit("in_shop")

    hooks = Hooks(enter_shop=enter_shop, pay=pay)
    shop = HilzersBarbershop(CAPACITY, SOFA, BARBERS, hooks)
    shop.start_barbers()
    results, results_lock = {}, threading.Lock()
    runner = ThreadRunner()
    spawn_customers(runner, shop, 14, results, results_lock)
    runner.join_all(timeout=20)

    tracker.assert_no_violations()
    assert tracker.max_concurrent("in_shop") <= CAPACITY


def test_sofa_capacity():
    tracker = OverlapTracker()

    def sit_on_sofa(cid):
        snapshot = tracker.enter("on_sofa")
        if snapshot["on_sofa"] > SOFA:
            tracker.violate(
                f"{snapshot['on_sofa']} customers on a sofa that seats {SOFA}"
            )

    def sit_in_chair(cid):
        time.sleep(0.003)  # dwell in the chair phase
        tracker.exit("on_sofa")  # the sofa seat frees when this hook returns

    hooks = Hooks(sit_on_sofa=sit_on_sofa, sit_in_chair=sit_in_chair)
    shop = HilzersBarbershop(CAPACITY, SOFA, BARBERS, hooks)
    shop.start_barbers()
    results, results_lock = {}, threading.Lock()
    runner = ThreadRunner()
    spawn_customers(runner, shop, 12, results, results_lock)
    runner.join_all(timeout=20)

    tracker.assert_no_violations()
    assert tracker.max_concurrent("on_sofa") <= SOFA


def test_concurrent_haircuts_up_to_barbers():
    # Two customers hold their chairs until the test has seen both seated
    # at once: with 2 barbers this MUST happen — a solution that
    # serializes the barbers never reaches it and fails here.
    tracker = OverlapTracker()
    all_in = threading.Event()

    def sit_in_chair(cid):
        tracker.enter("in_chair")
        all_in.wait(5)  # linger until both chairs are occupied together
        tracker.exit("in_chair")

    hooks = Hooks(sit_in_chair=sit_in_chair)
    shop = HilzersBarbershop(CAPACITY, SOFA, BARBERS, hooks)
    shop.start_barbers()
    results, results_lock = {}, threading.Lock()
    runner = ThreadRunner()
    spawn_customers(runner, shop, BARBERS, results, results_lock)
    eventually(
        lambda: runner.raise_worker_errors()
        or tracker.current("in_chair") == BARBERS,
        timeout=5,
        msg=f"never saw {BARBERS} concurrent haircuts — barbers must work "
        "in parallel",
    )
    all_in.set()
    runner.join_all(timeout=15)
    assert tracker.max_concurrent("in_chair") == BARBERS


def test_sofa_is_fifo():
    # Six customers take an extra-wide sofa 25 ms apart while the barber is
    # not yet working; once the barber starts, chairs must be offered in
    # sofa-seating order. ONE barber here: with several, two chairs are
    # granted concurrently and the hook-call order between them is an
    # honest race even for correct solutions — a single barber makes the
    # grant order (the property under test) observable.
    for trial in range(2):
        order = EventLog()
        hooks = Hooks(
            sit_on_sofa=lambda cid: order.record(f"sofa{cid}"),
            sit_in_chair=lambda cid: order.record(f"chair{cid}"),
        )
        shop = HilzersBarbershop(CAPACITY, 6, 1, hooks)
        results, results_lock = {}, threading.Lock()
        runner = ThreadRunner()
        spawn_customers(runner, shop, 6, results, results_lock, stagger=0.025)

        def sofa_full():
            runner.raise_worker_errors()
            return sum(1 for e in order.events() if e.startswith("sofa")) == 6

        eventually(sofa_full, timeout=5, msg="6 customers never made the sofa")
        time.sleep(0.15)
        runner.raise_worker_errors()
        assert not any(e.startswith("chair") for e in order.events()), (
            f"trial {trial}: a chair was taken before any barber existed"
        )

        shop.start_barbers()
        runner.join_all(timeout=10)
        sofa_order = [e[4:] for e in order.events() if e.startswith("sofa")]
        chair_order = [e[5:] for e in order.events() if e.startswith("chair")]
        assert chair_order == sofa_order, (
            f"trial {trial}: chair order {chair_order} != sofa order "
            f"{sofa_order} — the longest-seated customer goes first"
        )


def test_payment_pairing():
    # Customers one at a time: each pay(cid) must be answered by an
    # accept_payment before that customer_visit returns.
    log = EventLog()
    hooks = Hooks(
        pay=lambda cid: log.record(f"pay{cid}"),
        accept_payment=lambda bid: log.record("accept"),
    )
    shop = HilzersBarbershop(CAPACITY, SOFA, BARBERS, hooks)
    shop.start_barbers()
    for cid in range(4):
        served = assert_completes(
            lambda cid=cid: shop.customer_visit(cid),
            timeout=5,
            msg=f"customer {cid}'s visit never completed",
        )
        assert served is True
        log.record(f"return{cid}")

    events = log.events()
    for cid in range(4):
        pay_i = events.index(f"pay{cid}")
        ret_i = events.index(f"return{cid}")
        accepts_between = [
            e for e in events[pay_i + 1 : ret_i] if e == "accept"
        ]
        assert accepts_between, (
            f"customer {cid} returned without a barber accepting the "
            f"payment: {events}"
        )
    assert events.count("accept") == 4, "one accept_payment per customer"


def test_everyone_served_or_balked():
    # The stress test: 14 customers vs capacity 8, everything instrumented
    # at once — capacity, sofa, chair ceiling, register exclusivity.
    tracker = OverlapTracker()

    def enter_shop(cid):
        snapshot = tracker.enter("in_shop")
        if snapshot["in_shop"] > CAPACITY:
            tracker.violate(f"{snapshot['in_shop']} in shop of {CAPACITY}")

    def sit_on_sofa(cid):
        snapshot = tracker.enter("on_sofa")
        if snapshot["on_sofa"] > SOFA:
            tracker.violate(f"{snapshot['on_sofa']} on sofa of {SOFA}")

    def sit_in_chair(cid):
        snapshot = tracker.enter("in_chair")
        if snapshot["in_chair"] > BARBERS:
            tracker.violate(
                f"{snapshot['in_chair']} haircuts with {BARBERS} barbers"
            )
        time.sleep(0.005)
        tracker.exit("in_chair")
        tracker.exit("on_sofa")

    def pay(cid):
        tracker.exit("in_shop")

    def accept_payment(bid):
        snapshot = tracker.enter("register")
        if snapshot["register"] > 1:
            tracker.violate("two barbers at the one cash register")
        time.sleep(0.002)
        tracker.exit("register")

    hooks = Hooks(
        enter_shop=enter_shop,
        sit_on_sofa=sit_on_sofa,
        sit_in_chair=sit_in_chair,
        pay=pay,
        accept_payment=accept_payment,
    )
    shop = HilzersBarbershop(CAPACITY, SOFA, BARBERS, hooks)
    shop.start_barbers()
    results, results_lock = {}, threading.Lock()
    runner = ThreadRunner()
    spawn_customers(runner, shop, 14, results, results_lock)
    runner.join_all(timeout=20)

    tracker.assert_no_violations()
    with results_lock:
        assert len(results) == 14, "every visit must return True or False"
        served = sum(1 for v in results.values() if v)
    assert served >= CAPACITY, (
        f"only {served} of 14 served — at least {CAPACITY} always fit"
    )
    assert tracker.max_concurrent("in_shop") <= CAPACITY
    assert tracker.max_concurrent("on_sofa") <= SOFA
    assert tracker.max_concurrent("in_chair") <= BARBERS
    assert tracker.max_concurrent("register") <= 1
