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

from rwlock_writer_priority import WriterPriorityReadWriteLock


def _writer_convoy_trial():
    """The ambush: W1 queues, THEN late readers arrive, THEN W2 queues.

    Writer priority demands both writers finish before any late reader
    enters — even W2, who arrived after the readers. A one-for-one fair
    gate (koan 12) lets a reader slip in between W1 and W2 here.
    Returns the EventLog.
    """
    lock = WriterPriorityReadWriteLock()
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

    def writer(name):
        lock.writer_enter()
        log.record("writer_in")
        jitter(0.5)
        lock.writer_exit()
        log.record(f"{name}_done")

    # W1 arrives and blocks (incumbent readers still inside).
    w1_probe = assert_blocks(
        lambda: writer("w1"), msg="W1 must wait for the incumbent readers"
    )

    # Late readers arrive AFTER W1 — they must be barred already.
    def late_reader():
        lock.reader_enter()
        log.record("late_reader_in")
        lock.reader_exit()

    for i in range(3):
        runner.spawn(late_reader, name=f"late-reader-{i}")
    time.sleep(0.25)
    runner.raise_worker_errors()
    assert log.count("late_reader_in") == 0, (
        "readers entered while a writer was queued"
    )

    # W2 arrives AFTER the late readers. Priority says: still before them.
    w2_probe = assert_blocks(
        lambda: writer("w2"), msg="W2 must wait too (incumbents still reading)"
    )

    release_incumbents.set()
    assert w1_probe.wait(5), "W1 should run once the incumbents leave"
    assert w2_probe.wait(5), "W2 should run in the same writer convoy"
    eventually(
        lambda: log.count("late_reader_in") == 3,
        timeout=5,
        msg="the late readers never got in after the writers drained",
    )
    runner.join_all(timeout=5)
    assert log.count("writer_in") == 2
    # THE priority check: every writer entry precedes every late reader.
    log.assert_before("writer_in", "late_reader_in")
    return log


def test_readers_barred_while_writers_queued():
    for _ in range(5):
        _writer_convoy_trial()


def test_readers_run_after_writers_drain():
    log = _writer_convoy_trial()
    assert log.count("late_reader_in") == 3
    assert log.count("w1_done") == 1
    assert log.count("w2_done") == 1


def test_concurrency_preserved():
    """No writer around: four readers must still hold the lock together."""
    lock = WriterPriorityReadWriteLock()
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
    lock = WriterPriorityReadWriteLock()
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
    lock = WriterPriorityReadWriteLock()
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


def test_writers_exclude_each_other():
    lock = WriterPriorityReadWriteLock()
    assert_completes(lock.writer_enter, timeout=2, msg="first writer entering")
    probe = assert_blocks(
        lock.writer_enter, msg="a second writer must wait for the first"
    )
    lock.writer_exit()
    assert probe.wait(5), "the second writer should enter once the first leaves"
    lock.writer_exit()


def test_invariant_stress():
    """Safety under churn — writers alone, readers overlapping.

    Writer streams are finite, so readers can't starve forever here; the
    point is that snapshots never show a writer sharing the room.
    """
    lock = WriterPriorityReadWriteLock()
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
