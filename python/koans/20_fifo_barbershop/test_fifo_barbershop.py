import random
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

from fifo_barbershop import FifoBarbershop


# The first four tests re-check everything koan 19 demanded — the FIFO
# shop must not lose any of those properties. The last one checks the new
# promise: service order == arrival order.


def test_haircut_pairing():
    # 10 customers stream through a 4-seat shop (a test-side gate keeps at
    # most 4 in flight, so nobody balks). Every cut_hair() must overlap
    # exactly one customer's get_hair_cut().
    shop = FifoBarbershop(4)
    tracker = OverlapTracker()
    results = []
    results_lock = threading.Lock()
    gate = threading.Semaphore(4)
    runner = ThreadRunner()

    def cut_hair():
        # NB: runs on the user's barber thread — never raises, only records.
        with tracker.section("cutting"):
            deadline = time.monotonic() + 5
            while tracker.current("being_cut") < 1 and time.monotonic() < deadline:
                time.sleep(0.001)
            if tracker.current("being_cut") != 1:
                tracker.violate(
                    "cut_hair ran without exactly one customer in the chair"
                )
            else:
                time.sleep(0.002)
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
    assert results == [True] * 10
    assert tracker.max_concurrent("being_cut") == 1


def test_balk_when_full():
    n = 4
    shop = FifoBarbershop(n)
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
    eventually(lambda: cuts.current("cut") == 1, timeout=5)
    time.sleep(0.3)  # let all n customers finish checking in

    extra = assert_completes(
        lambda: shop.customer_visit(lambda: None),
        timeout=2,
        msg=f"customer {n + 1} should balk immediately when the shop is full",
    )
    assert extra is False, f"customer {n + 1} should have balked with False"

    haircuts_may_finish.set()
    runner.join_all(timeout=10)
    with results_lock:
        assert results == [True] * n


def test_barber_sleeps_when_no_customers():
    shop = FifoBarbershop(4)
    cuts = []
    shop.start_barber(lambda: cuts.append(1))
    time.sleep(0.3)
    assert cuts == [], "the barber cut hair with no customer in the shop"


def test_stress_random_arrivals():
    n = 4
    shop = FifoBarbershop(n)
    tracker = OverlapTracker()
    results = []
    results_lock = threading.Lock()
    runner = ThreadRunner()

    def cut_hair():
        with tracker.section("cutting", dwell=0.001):
            pass

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
    assert served + sum(1 for r in results if not r) == 20
    assert served >= n
    assert tracker.max_concurrent("being_cut") <= 1


def test_served_in_arrival_order():
    # Six customers arrive 25 ms apart — far wider than any registration
    # race — while the barber is not yet working. Once everyone is waiting,
    # the barber starts: haircuts must then happen in arrival order.
    #
    # Fine print: CPython's stdlib semaphores happen to wake waiters FIFO,
    # so a koan-19-style shop (one shared semaphore for everyone) can pass
    # this test here. It is still wrong — no spec grants that order. The
    # C++ twin of this koan has no such safety net. See the README.
    for trial in range(3):
        shop = FifoBarbershop(8)
        order = EventLog()
        runner = ThreadRunner()

        def customer(i, shop=shop, order=order):
            served = shop.customer_visit(lambda i=i: order.record(f"c{i}"))
            assert served is True

        for i in range(6):
            runner.spawn(customer, i, name=f"customer-{i}")
            time.sleep(0.025)

        time.sleep(0.15)  # everyone is now registered and waiting
        runner.raise_worker_errors()
        assert order.events() == [], (
            f"trial {trial}: someone was served before the barber started"
        )

        shop.start_barber(lambda: None)
        runner.join_all(timeout=10)
        assert order.events() == [f"c{i}" for i in range(6)], (
            f"trial {trial}: service order {order.events()} != arrival order "
            "— the barber must call customers in FIFO order"
        )
