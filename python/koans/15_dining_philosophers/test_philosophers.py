import threading
import time

from koan_utils import (
    EventLog,
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from philosophers import Table

N = 5


def neighbors(i, n=N):
    return ((i - 1) % n, (i + 1) % n)


def run_dinner(meals, think_ms, eat_ms, join_timeout):
    """Run a full dinner; returns (tracker, log) for the caller's asserts."""
    table = Table(N)
    tracker = OverlapTracker()
    log = EventLog()
    runner = ThreadRunner()

    def philosopher(i):
        for _ in range(meals):
            if think_ms:
                jitter(think_ms)
            table.get_forks(i)
            snapshot = tracker.enter(f"eat:{i}")
            for j in neighbors(i):
                if snapshot.get(f"eat:{j}", 0) > 0:
                    tracker.violate(
                        f"philosophers {i} and {j} are neighbors and were "
                        "eating at the same time"
                    )
            if eat_ms:
                jitter(eat_ms)
            tracker.exit(f"eat:{i}")
            table.put_forks(i)
            log.record(f"meal:{i}")

    for i in range(N):
        runner.spawn(philosopher, i, name=f"phil-{i}")
    runner.join_all(timeout=join_timeout)
    return tracker, log


def test_neighbors_never_eat_together():
    tracker, _ = run_dinner(meals=20, think_ms=2, eat_ms=1, join_timeout=20)
    tracker.assert_no_violations()


def test_hungry_neighbor_blocks_until_forks_return():
    """Philosophers 0 and 1 share a fork: while 0 eats, 1 must wait."""
    table = Table(N)
    assert_completes(
        lambda: table.get_forks(0), timeout=2,
        msg="get_forks at an empty table must not block",
    )
    probe = assert_blocks(
        lambda: table.get_forks(1),
        msg="philosopher 1 must wait while neighbor 0 holds their shared fork",
    )
    table.put_forks(0)
    assert probe.wait(5), "philosopher 1 should get the forks once 0 puts them down"
    table.put_forks(1)


def test_two_nonadjacent_philosophers_eat_together():
    """Seats 0 and 2 share no fork; the table must let them eat at once."""
    table = Table(N)
    tracker = OverlapTracker()
    done = threading.Event()
    runner = ThreadRunner()

    def philosopher(i):
        table.get_forks(i)
        tracker.enter(f"eat:{i}")
        done.wait(5)  # linger so the overlap is observable
        tracker.exit(f"eat:{i}")
        table.put_forks(i)

    runner.spawn(philosopher, 0, name="phil-0")
    runner.spawn(philosopher, 2, name="phil-2")
    eventually(
        lambda: tracker.current("eat:0") == 1 and tracker.current("eat:2") == 1,
        timeout=5,
        msg="philosophers 0 and 2 share no fork but never ate together — "
        "the table over-serializes",
    )
    done.set()
    runner.join_all(timeout=5)


def test_no_deadlock_when_everyone_is_hungry():
    """Zero think time, everyone grabbing at once, and an adversarial
    scheduler: every semaphore acquire is followed by a forced nap, so any
    window between "grab first fork" and "grab second fork" WILL be hit.
    The classic circular wedge has nowhere to hide."""
    meals = 30
    original_acquire = threading.Semaphore.acquire

    def adversarial_acquire(self, *args, **kwargs):
        result = original_acquire(self, *args, **kwargs)
        time.sleep(0.0002)  # yield the CPU right after every acquire
        return result

    threading.Semaphore.acquire = adversarial_acquire
    try:
        tracker, log = run_dinner(
            meals=meals, think_ms=0, eat_ms=0, join_timeout=15
        )
    finally:
        threading.Semaphore.acquire = original_acquire
    tracker.assert_no_violations()
    for i in range(N):
        assert log.count(f"meal:{i}") == meals


def test_everyone_eats_and_meals_overlap():
    meals = 20
    tracker, log = run_dinner(meals=meals, think_ms=1, eat_ms=0.5, join_timeout=20)
    tracker.assert_no_violations()
    for i in range(N):
        assert log.count(f"meal:{i}") == meals, (
            f"philosopher {i} finished only {log.count(f'meal:{i}')} of "
            f"{meals} meals"
        )
    assert tracker.max_combined() >= 2, (
        f"in {N * meals} meals no two philosophers ever ate at the same "
        "time — the table over-serializes"
    )
