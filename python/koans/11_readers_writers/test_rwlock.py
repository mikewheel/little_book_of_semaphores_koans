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

from rwlock import ReadWriteLock


def test_readers_share():
    """Four readers must be able to hold the lock at the same time."""
    lock = ReadWriteLock()
    tracker = OverlapTracker()
    all_in = threading.Event()
    runner = ThreadRunner()

    def reader():
        lock.reader_enter()
        tracker.enter("read")
        all_in.wait(5)  # linger until everyone is inside together
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
    lock = ReadWriteLock()
    log = EventLog()
    runner = ThreadRunner()
    assert_completes(lock.writer_enter, timeout=2, msg="writer entering an idle lock")

    probe = assert_blocks(
        lambda: (lock.reader_enter(), log.record("probe_reader_in")),
        msg="a reader must wait while a writer holds the lock",
    )

    def reader(i):
        lock.reader_enter()
        log.record(f"reader{i}_in")
        lock.reader_exit()

    for i in range(2):
        runner.spawn(reader, i)
    time.sleep(0.25)  # give them every chance to sneak in
    runner.raise_worker_errors()
    assert log.events() == [], f"readers entered over a live writer: {log.events()}"

    lock.writer_exit()
    assert probe.wait(5), "the blocked reader should enter once the writer leaves"
    eventually(lambda: log.count("reader0_in") + log.count("reader1_in") == 2, timeout=5)
    lock.reader_exit()  # the probe's reader
    runner.join_all(timeout=5)


def test_readers_exclude_writer():
    lock = ReadWriteLock()
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
    lock = ReadWriteLock()
    assert_completes(lock.writer_enter, timeout=2, msg="first writer entering")
    probe = assert_blocks(
        lock.writer_enter, msg="a second writer must wait for the first"
    )
    lock.writer_exit()
    assert probe.wait(5), "the second writer should enter once the first leaves"
    lock.writer_exit()


def test_invariant_stress():
    """Snapshots taken at every entry: writers are always alone."""
    lock = ReadWriteLock()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def reader():
        for _ in range(12):
            jitter()
            lock.reader_enter()
            snap = tracker.enter("read")
            if snap.get("write", 0):
                tracker.violate(f"reader entered alongside a writer: {snap}")
            jitter(1.0)
            tracker.exit("read")
            lock.reader_exit()

    def writer():
        for _ in range(8):
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
    assert tracker.max_concurrent("read") >= 2, (
        "readers never actually overlapped — suspiciously serialized"
    )
