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

from rwlock_fair import NoStarveReadWriteLock


def _no_starvation_trial():
    """One full scene: incumbents in, writer queues, latecomers must wait.

    Returns the EventLog for extra assertions.
    """
    lock = NoStarveReadWriteLock()
    log = EventLog()
    release_incumbents = threading.Event()
    runner = ThreadRunner()

    def incumbent():
        lock.reader_enter()
        log.record("incumbent_in")
        release_incumbents.wait(10)
        lock.reader_exit()

    runner.spawn(incumbent, name="incumbent-0")
    runner.spawn(incumbent, name="incumbent-1")
    log.wait_for_count("incumbent_in", 2, timeout=5)

    # A writer arrives. It must block — two readers are mid-read.
    writer_probe = assert_blocks(
        lambda: (lock.writer_enter(), log.record("writer_in")),
        msg="the writer must wait for the incumbent readers to finish",
    )

    # Three readers arrive AFTER the writer. Fairness says they queue.
    def late_reader(i):
        lock.reader_enter()
        log.record("late_reader_in")
        lock.reader_exit()

    for i in range(3):
        runner.spawn(late_reader, i, name=f"late-reader-{i}")
    time.sleep(0.3)  # every chance to jump the queue
    runner.raise_worker_errors()
    assert log.count("late_reader_in") == 0, (
        "readers that arrived after a waiting writer entered before it"
    )

    # Incumbents leave; the writer — not the latecomers — goes next.
    release_incumbents.set()
    assert writer_probe.wait(5), (
        "the writer should enter once the incumbent readers leave"
    )
    assert log.count("late_reader_in") == 0, (
        "a late reader entered before the queued writer"
    )

    lock.writer_exit()
    eventually(
        lambda: log.count("late_reader_in") == 3,
        timeout=5,
        msg="the late readers never got in after the writer finished",
    )
    runner.join_all(timeout=5)
    log.assert_before("writer_in", "late_reader_in")
    return log


def test_writer_not_starved():
    for _ in range(5):
        _no_starvation_trial()


def test_late_readers_eventually_get_in():
    log = _no_starvation_trial()
    assert log.count("late_reader_in") == 3
    assert log.count("writer_in") == 1


def test_readers_share():
    """No writer around: four readers must still hold the lock together."""
    lock = NoStarveReadWriteLock()
    tracker = OverlapTracker()
    all_in = threading.Event()
    runner = ThreadRunner()

    def reader():
        lock.reader_enter()
        tracker.enter("read")
        all_in.wait(5)
        tracker.exit("read")
        lock.reader_exit()

    for _ in range(4):
        runner.spawn(reader)
    eventually(
        lambda: tracker.current("read") == 4,
        timeout=5,
        msg=f"only {tracker.current('read')} of 4 readers inside together — "
        "readers are being serialized",
    )
    all_in.set()
    runner.join_all(timeout=5)


def test_writer_excludes_readers():
    lock = NoStarveReadWriteLock()
    log = EventLog()
    assert_completes(lock.writer_enter, timeout=2, msg="writer entering an idle lock")
    probe = assert_blocks(
        lambda: (lock.reader_enter(), log.record("reader_in")),
        msg="a reader must wait while a writer holds the lock",
    )
    lock.writer_exit()
    assert probe.wait(5), "the reader should enter once the writer leaves"
    lock.reader_exit()


def test_readers_exclude_writer():
    lock = NoStarveReadWriteLock()
    log = EventLog()
    for _ in range(2):
        assert_completes(lock.reader_enter, timeout=2, msg="reader entering an idle lock")
    probe = assert_blocks(
        lambda: (lock.writer_enter(), log.record("writer_in")),
        msg="a writer must wait while readers hold the lock",
    )
    lock.reader_exit()
    time.sleep(0.25)
    assert log.count("writer_in") == 0, (
        "the writer entered while one reader was still inside"
    )
    lock.reader_exit()
    assert probe.wait(5), "the writer should enter once the LAST reader leaves"
    lock.writer_exit()


def test_invariant_stress():
    """Safety under churn: every writer-entry snapshot shows an empty room."""
    lock = NoStarveReadWriteLock()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def reader():
        for _ in range(10):
            jitter()
            lock.reader_enter()
            snap = tracker.enter("read")
            if snap.get("write", 0):
                tracker.violate(f"reader entered alongside a writer: {snap}")
            jitter(1.0)
            tracker.exit("read")
            lock.reader_exit()

    def writer():
        for _ in range(6):
            jitter()
            lock.writer_enter()
            snap = tracker.enter("write")
            if snap.get("read", 0) or snap.get("write") != 1:
                tracker.violate(f"writer entered a non-empty room: {snap}")
            jitter(1.0)
            tracker.exit("write")
            lock.writer_exit()

    for _ in range(6):
        runner.spawn(reader)
    for _ in range(3):
        runner.spawn(writer)
    runner.join_all(timeout=30)
    tracker.assert_no_violations()
    assert tracker.max_concurrent("write") <= 1
