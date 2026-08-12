import threading
import time

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    jitter,
)

from mutex import Mutex


class RacyCounter:
    """An increment with a deliberately widened read-modify-write window."""

    def __init__(self):
        self.value = 0

    def increment(self, nap_every, i):
        temp = self.value
        if i % nap_every == 0:
            time.sleep(0.0005)  # invite the scheduler to interleave
        self.value = temp + 1


def test_no_lost_updates():
    mutex = Mutex()
    counter = RacyCounter()
    n_threads, n_iters = 4, 50
    runner = ThreadRunner()

    def worker():
        for i in range(n_iters):
            mutex.acquire()
            counter.increment(nap_every=10, i=i)
            mutex.release()

    for _ in range(n_threads):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert counter.value == n_threads * n_iters, (
        f"lost updates: expected {n_threads * n_iters}, got {counter.value} — "
        "the critical section is not exclusive"
    )


def test_critical_section_is_exclusive():
    mutex = Mutex()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def worker():
        for _ in range(25):
            mutex.acquire()
            with tracker.section("cs"):
                jitter(0.5)
            mutex.release()

    for _ in range(4):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("cs") == 1, (
        f"{tracker.max_concurrent('cs')} threads were inside the critical "
        "section at once"
    )


def test_second_acquire_blocks_until_release():
    mutex = Mutex()
    assert_completes(mutex.acquire, timeout=2, msg="first acquire must not block")
    probe = assert_blocks(
        mutex.acquire, msg="second acquire must block while the mutex is held"
    )
    mutex.release()
    assert probe.wait(5), "waiter should acquire the mutex after release"
    mutex.release()


def test_works_for_many_threads():
    mutex = Mutex()
    counter = RacyCounter()
    runner = ThreadRunner()

    def worker():
        for i in range(20):
            mutex.acquire()
            counter.increment(nap_every=7, i=i)
            mutex.release()

    for _ in range(10):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert counter.value == 200


def test_sanity_racy_counter_actually_races_without_a_mutex():
    """Meta-test: proves the test rig can detect a broken mutex.

    If this one ever fails, the tests above have lost their teeth; nothing
    about YOUR code is checked here.
    """
    counter = RacyCounter()
    runner = ThreadRunner()

    def worker():
        for i in range(50):
            counter.increment(nap_every=10, i=i)

    for _ in range(4):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert counter.value < 200, (
        "the racy counter did not race; scheduling assumptions are off"
    )
