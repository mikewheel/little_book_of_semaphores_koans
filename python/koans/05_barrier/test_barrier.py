import threading
import time

from koan_utils import EventLog, ThreadRunner, eventually, jitter

from barrier import Barrier


def test_nobody_passes_before_the_last_arrival():
    n = 5
    barrier = Barrier(n)
    passed = EventLog()
    runner = ThreadRunner()

    def worker(i):
        barrier.wait()
        passed.record(f"t{i}")

    for i in range(n - 1):
        runner.spawn(worker, i)
    time.sleep(0.3)  # give the early arrivals every chance to leak through
    runner.raise_worker_errors()
    assert passed.events() == [], (
        f"{len(passed.events())} thread(s) passed the barrier before all "
        f"{n} arrived"
    )
    runner.spawn(worker, n - 1)  # the nth arrival springs the door
    runner.join_all(timeout=5)
    assert len(passed.events()) == n, (
        f"only {len(passed.events())} of {n} threads passed — "
        "did the nth arrival wake exactly one sleeper?"
    )


def test_all_threads_pass_when_arrivals_are_staggered():
    n = 4
    barrier = Barrier(n)
    passed = EventLog()
    runner = ThreadRunner()

    def worker(i):
        time.sleep(0.01 * i)  # arrive one by one
        barrier.wait()
        passed.record(f"t{i}")

    for i in range(n):
        runner.spawn(worker, i)
    runner.join_all(timeout=5)
    assert len(passed.events()) == n


def test_late_straggler_releases_everyone():
    n = 3
    barrier = Barrier(n)
    passed = EventLog()
    runner = ThreadRunner()

    def worker(i):
        barrier.wait()
        passed.record(f"t{i}")

    runner.spawn(worker, 0)
    runner.spawn(worker, 1)
    time.sleep(0.2)
    assert passed.events() == []
    runner.spawn(worker, 2)
    eventually(
        lambda: len(passed.events()) == n,
        timeout=5,
        msg="the last arrival did not release all waiting threads",
    )
    runner.join_all(timeout=5)


def test_stress_various_sizes():
    for n in (2, 7, 16):
        barrier = Barrier(n)
        passed = EventLog()
        runner = ThreadRunner()

        def worker(i, barrier=barrier, passed=passed):
            jitter()
            barrier.wait()
            passed.record(f"t{i}")

        for i in range(n):
            runner.spawn(worker, i)
        runner.join_all(timeout=10)
        assert len(passed.events()) == n, f"n={n}: not everyone passed"
