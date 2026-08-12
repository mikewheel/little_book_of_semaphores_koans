import threading

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from multiplex import Multiplex


def test_never_more_than_n_inside():
    n = 4
    plex = Multiplex(n)
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def worker():
        for _ in range(20):
            plex.enter()
            with tracker.section("room"):
                jitter(1.0)
            plex.exit()

    for _ in range(12):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("room") <= n, (
        f"room capacity {n} exceeded: saw {tracker.max_concurrent('room')}"
    )


def test_n_threads_can_be_inside_together():
    """A mutex in a trench coat must not pass: full concurrency is required."""
    n = 4
    plex = Multiplex(n)
    tracker = OverlapTracker()
    all_in = threading.Event()
    runner = ThreadRunner()

    def worker():
        plex.enter()
        tracker.enter("room")
        all_in.wait(5)  # linger until everyone is inside together
        tracker.exit("room")
        plex.exit()

    for _ in range(n):
        runner.spawn(worker)
    eventually(
        lambda: tracker.current("room") == n,
        timeout=5,
        msg=f"only {tracker.current('room')} of {n} threads made it inside "
        "concurrently — the multiplex over-serializes",
    )
    all_in.set()
    runner.join_all(timeout=5)


def test_thread_n_plus_1_blocks_then_proceeds_after_an_exit():
    n = 3
    plex = Multiplex(n)
    for _ in range(n):
        assert_completes(plex.enter, timeout=2)
    probe = assert_blocks(
        plex.enter, msg=f"entry {n + 1} must block while {n} are inside"
    )
    plex.exit()
    assert probe.wait(5), "a waiter should get the freed slot"
    for _ in range(n):
        plex.exit()


def test_capacity_one_degenerates_to_a_mutex():
    plex = Multiplex(1)
    assert_completes(plex.enter, timeout=2)
    probe = assert_blocks(plex.enter)
    plex.exit()
    assert probe.wait(5)
    plex.exit()


def test_stress_various_sizes():
    for n in (2, 5, 8):
        plex = Multiplex(n)
        tracker = OverlapTracker()
        runner = ThreadRunner()

        def worker(plex=plex, tracker=tracker):
            for _ in range(10):
                plex.enter()
                with tracker.section("room"):
                    jitter(0.5)
                plex.exit()

        for _ in range(n * 3):
            runner.spawn(worker)
        runner.join_all(timeout=30)
        assert tracker.max_concurrent("room") <= n
