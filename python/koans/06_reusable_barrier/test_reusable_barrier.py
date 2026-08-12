import time

from koan_utils import EventLog, ThreadRunner, jitter

from reusable_barrier import ReusableBarrier


def assert_rounds_atomic(log, n, rounds):
    """Per round r: all n arrivals precede every departure of round r, and
    nobody arrives at round r+1 before everyone has arrived at round r."""
    for r in range(rounds):
        arrive, depart = f"arrive:{r}", f"depart:{r}"
        assert log.count(arrive) == n, (
            f"round {r}: expected {n} arrivals, saw {log.count(arrive)}"
        )
        assert log.count(depart) == n, (
            f"round {r}: expected {n} departures, saw {log.count(depart)}"
        )
        log.assert_before(arrive, depart)
        if r + 1 < rounds:
            log.assert_before(arrive, f"arrive:{r + 1}")


def run_rounds(n, rounds, join_timeout=15):
    barrier = ReusableBarrier(n)
    log = EventLog()
    runner = ThreadRunner()

    def worker():
        for r in range(rounds):
            jitter()
            log.record(f"arrive:{r}")
            barrier.wait()
            log.record(f"depart:{r}")
            jitter()

    for _ in range(n):
        runner.spawn(worker)
    runner.join_all(timeout=join_timeout)
    return log


def test_rounds_are_atomic():
    n, rounds = 4, 8
    log = run_rounds(n, rounds)
    assert_rounds_atomic(log, n, rounds)


def test_nobody_leaks_through_early():
    n = 4
    barrier = ReusableBarrier(n)
    departed = EventLog()
    runner = ThreadRunner()

    def worker(r):
        barrier.wait()
        departed.record(f"depart:{r}")

    for r in range(2):  # two consecutive rounds of the same barrier object
        for _ in range(n - 1):
            runner.spawn(worker, r)
        time.sleep(0.25)  # plenty of time to misbehave
        runner.raise_worker_errors()
        assert departed.count(f"depart:{r}") == 0, (
            f"round {r}: {departed.count(f'depart:{r}')} thread(s) got past "
            f"the barrier with only {n - 1} of {n} arrived"
        )
        runner.spawn(worker, r)  # the nth arrival springs the door
        runner.join_all(timeout=5)
        assert departed.count(f"depart:{r}") == n


def test_many_rounds_stress():
    # Uses the split-phase API (work between phase1 and phase2 is allowed),
    # which is where a resettable-in-name-only barrier lets a fast thread
    # lap the field.
    n, rounds = 6, 40
    barrier = ReusableBarrier(n)
    log = EventLog()
    runner = ThreadRunner()

    def worker():
        for r in range(rounds):
            jitter(1.0)
            log.record(f"arrive:{r}")
            barrier.phase1()
            jitter(3.0)  # some "work" at the critical point between phases
            barrier.phase2()
            log.record(f"depart:{r}")

    for _ in range(n):
        runner.spawn(worker)
    runner.join_all(timeout=15)
    total = sum(log.count(f"depart:{r}") for r in range(rounds))
    assert total == n * rounds, f"expected {n * rounds} departures, saw {total}"
    assert_rounds_atomic(log, n, rounds)


def test_works_for_two_batches_of_waits():
    n = 3
    barrier = ReusableBarrier(n)
    for _ in range(2):  # batch of n waits, plain join, then batch again
        runner = ThreadRunner()
        for _ in range(n):
            runner.spawn(barrier.wait)
        runner.join_all(timeout=5)
