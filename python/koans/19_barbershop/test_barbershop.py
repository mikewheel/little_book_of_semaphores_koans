import random
import threading
import time

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_completes,
    eventually,
    jitter,
)

from barbershop import Barbershop


# Note: this shop makes NO promise about serving customers in arrival
# order — any waiting customer may be called next. Koan 20 adds FIFO.


def test_haircut_pairing():
    # 10 customers stream through a 4-seat shop (a test-side gate keeps at
    # most 4 in flight, so nobody balks). Every cut_hair() must overlap
    # exactly one customer's get_hair_cut() — never two chairs at once, and
    # never a cut that races ahead of the customer it belongs to.
    shop = Barbershop(4)
    tracker = OverlapTracker()
    results = []
    results_lock = threading.Lock()
    gate = threading.Semaphore(4)  # test-side: keeps the shop below balking
    runner = ThreadRunner()

    def cut_hair():
        # NB: this runs on the user's barber thread, so it must never
        # raise — it records violations for the test thread to assert on.
        with tracker.section("cutting"):
            # The paired customer must show up in the chair while we cut...
            deadline = time.monotonic() + 5
            while tracker.current("being_cut") < 1 and time.monotonic() < deadline:
                time.sleep(0.001)
            if tracker.current("being_cut") != 1:
                tracker.violate(
                    "cut_hair ran without exactly one customer in the chair"
                )
            else:
                time.sleep(0.002)
                # ...and still be the only one there while we're cutting.
                if tracker.current("being_cut") != 1:
                    tracker.violate(
                        "a second customer was in a chair before the previous "
                        "haircut was fully done"
                    )

    def get_hair_cut():
        snapshot = tracker.enter("being_cut")
        if snapshot["being_cut"] > 1:
            tracker.violate("two customers being cut at once")
        time.sleep(0.010)
        tracker.exit("being_cut")

    def customer():
        gate.acquire()
        try:
            served = shop.customer_visit(get_hair_cut)
        finally:
            gate.release()
        with results_lock:
            results.append(served)

    shop.start_barber(cut_hair)
    for _ in range(10):
        runner.spawn(customer)
    runner.join_all(timeout=15)

    tracker.assert_no_violations()
    assert results == [True] * 10, f"every gated customer should be served: {results}"
    assert tracker.max_concurrent("being_cut") == 1
    assert tracker.max_concurrent("cutting") == 1


def test_balk_when_full():
    n = 4
    shop = Barbershop(n)
    haircuts_may_finish = threading.Event()
    cuts = OverlapTracker()
    results = []
    results_lock = threading.Lock()
    runner = ThreadRunner()

    def cut_hair():
        cuts.enter("cut")
        assert haircuts_may_finish.wait(10), "test event never released"
        cuts.exit("cut")

    def customer():
        served = shop.customer_visit(lambda: None)
        with results_lock:
            results.append(served)

    shop.start_barber(cut_hair)
    for _ in range(n):
        runner.spawn(customer)
    # Wait until the shop is genuinely wedged: barber mid-cut, everyone in.
    eventually(lambda: cuts.current("cut") == 1, timeout=5)
    time.sleep(0.3)  # let all n customers finish checking in

    # The (n+1)th customer must bounce straight off the full shop: a prompt
    # False, not a blocked call.
    extra = assert_completes(
        lambda: shop.customer_visit(lambda: None),
        timeout=2,
        msg=f"customer {n + 1} should balk immediately when the shop is full",
    )
    assert extra is False, f"customer {n + 1} should have balked with False"

    haircuts_may_finish.set()  # unblock the barber → the n insiders drain
    runner.join_all(timeout=10)
    with results_lock:
        assert results == [True] * n, f"all {n} insiders should be served: {results}"


def test_barber_sleeps_when_no_customers():
    shop = Barbershop(4)
    cuts = []
    shop.start_barber(lambda: cuts.append(1))
    time.sleep(0.3)  # an empty shop…
    assert cuts == [], "the barber cut hair with no customer in the shop"


def test_stress_random_arrivals():
    n = 4
    shop = Barbershop(n)
    tracker = OverlapTracker()
    results = []
    cut_count = [0]
    results_lock = threading.Lock()
    runner = ThreadRunner()

    def cut_hair():
        with tracker.section("cutting", dwell=0.001):
            with results_lock:
                cut_count[0] += 1

    def get_hair_cut():
        snapshot = tracker.enter("being_cut")
        if snapshot["being_cut"] > 1:
            tracker.violate("two customers being cut at once")
        time.sleep(0.001)
        tracker.exit("being_cut")

    def customer():
        time.sleep(random.uniform(0, 0.05))
        jitter()
        served = shop.customer_visit(get_hair_cut)
        with results_lock:
            results.append(served)

    shop.start_barber(cut_hair)
    for _ in range(20):
        runner.spawn(customer)
    runner.join_all(timeout=15)

    tracker.assert_no_violations()
    served = sum(1 for r in results if r)
    balked = sum(1 for r in results if not r)
    assert served + balked == 20, "every visit must return True or False"
    assert served >= n, f"only {served} customers served out of 20"
    assert tracker.max_concurrent("being_cut") <= 1
    with results_lock:
        assert cut_count[0] == served, (
            f"{cut_count[0]} cuts for {served} served customers — cuts and "
            "customers must pair 1:1"
        )
