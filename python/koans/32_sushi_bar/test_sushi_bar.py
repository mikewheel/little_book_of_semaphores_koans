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

from sushi_bar import SushiBar


def test_seats_capacity():
    bar = SushiBar(seats=5)
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def diner():
        def eat():
            with tracker.section("seat"):
                jitter(1.0)

        for _ in range(10):
            jitter()
            bar.dine(eat)

    for _ in range(10):
        runner.spawn(diner)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("seat") <= 5, (
        f"more diners than seats: {tracker.max_concurrent('seat')}"
    )


def test_no_wait_when_partly_full():
    bar = SushiBar()
    log = EventLog()
    runner = ThreadRunner()
    hold = threading.Event()

    def incumbent():
        def eat():
            log.record("incumbent_seated")
            hold.wait(10)

        bar.dine(eat)

    for _ in range(3):
        runner.spawn(incumbent)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("incumbent_seated") == 3,
        timeout=5,
    )

    # 3 of 5 seats taken, no must-wait: the 4th customer sits immediately.
    assert_completes(
        lambda: bar.dine(lambda: log.record("fourth_seated")),
        timeout=2,
        msg="a customer must be seated immediately while seats are free "
        "and the bar never filled",
    )
    hold.set()
    runner.join_all(timeout=5)


def test_full_bar_forces_cohort_wait():
    for _ in range(5):
        _cohort_trial()


def _cohort_trial():
    bar = SushiBar()
    log = EventLog()
    tracker = OverlapTracker()
    runner = ThreadRunner()
    first_two = threading.Event()
    last_three = threading.Event()

    # Five incumbents fill the bar; the party is now closed.
    def incumbent(gate):
        def eat():
            with tracker.section("seat"):
                log.record("incumbent_seated")
                gate.wait(10)

        bar.dine(eat)

    for _ in range(2):
        runner.spawn(incumbent, first_two)
    for _ in range(3):
        runner.spawn(incumbent, last_three)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("incumbent_seated") == 5,
        timeout=5,
    )

    # Two customers arrive at the full bar: they must wait.
    def waiter():
        log.record("waiter_arrived")

        def eat():
            with tracker.section("seat"):
                log.record("waiter_seated")
                time.sleep(0.01)

        bar.dine(eat)

    for _ in range(2):
        runner.spawn(waiter)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("waiter_arrived") == 2,
        timeout=5,
    )
    time.sleep(0.25)
    assert log.count("waiter_seated") == 0, "customers sat down at a full bar"

    # Two incumbents leave. Seats are free — but the bar hasn't emptied,
    # so the waiters must STILL be waiting.
    first_two.set()
    time.sleep(0.3)
    runner.raise_worker_errors()
    assert log.count("waiter_seated") == 0, (
        "a waiter took a freed seat before the bar emptied — must-wait "
        "mode ended too early"
    )

    # The last three leave; the whole waiting cohort sits together. A wave
    # of fresh arrivals lands at the same moment and must not overfill
    # the bar (nor be seated ahead of the cohort's claim to exist).
    last_three.set()

    def newcomer():
        def eat():
            with tracker.section("seat"):
                log.record("newcomer_seated")
                time.sleep(0.01)

        bar.dine(eat)

    for _ in range(6):
        runner.spawn(newcomer)
    log.wait_for_count("waiter_seated", 2, timeout=5)
    runner.join_all(timeout=10)
    assert tracker.max_concurrent("seat") <= 5, (
        f"the bar overfilled during the reseat: "
        f"{tracker.max_concurrent('seat')} diners at once"
    )


def test_mode_resets():
    bar = SushiBar()
    log = EventLog()
    runner = ThreadRunner()
    incumbents_hold = threading.Event()
    cohort_hold = threading.Event()

    def incumbent():
        def eat():
            log.record("incumbent_seated")
            incumbents_hold.wait(10)

        bar.dine(eat)

    for _ in range(5):
        runner.spawn(incumbent)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("incumbent_seated") == 5,
        timeout=5,
    )

    # Two arrive at the full bar and wait out the party.
    def waiter():
        def eat():
            log.record("waiter_seated")
            cohort_hold.wait(10)

        bar.dine(eat)

    for _ in range(2):
        runner.spawn(waiter)
    time.sleep(0.2)
    runner.raise_worker_errors()
    assert log.count("waiter_seated") == 0

    # The bar empties; the 2-person cohort seats; must-wait mode is over.
    incumbents_hold.set()
    log.wait_for_count("waiter_seated", 2, timeout=5)

    # 2 of 5 seats taken and the mode reset: a newcomer sits immediately.
    assert_completes(
        lambda: bar.dine(lambda: log.record("newcomer_seated")),
        timeout=2,
        msg="after the bar emptied and the cohort sat down, a newcomer "
        "must be seated immediately — must-wait mode failed to reset",
    )
    cohort_hold.set()
    runner.join_all(timeout=10)


def test_stress_capacity_and_totals():
    bar = SushiBar()
    tracker = OverlapTracker()
    log = EventLog()
    runner = ThreadRunner()

    def diner():
        def eat():
            with tracker.section("seat"):
                log.record("ate")
                jitter(2.0)

        for _ in range(8):
            jitter(2.0)
            bar.dine(eat)

    for _ in range(25):
        runner.spawn(diner)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("seat") <= 5, (
        f"more diners than seats under load: {tracker.max_concurrent('seat')}"
    )
    assert log.count("ate") == 200, f"only {log.count('ate')} of 200 meals served"
