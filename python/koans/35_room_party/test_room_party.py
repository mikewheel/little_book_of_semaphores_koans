import collections
import threading
import time

from koan_utils import EventLog, ThreadRunner, assert_blocks, assert_completes, eventually

from room_party import Room

THRESHOLD = 5


def make_room(hold):
    """A Room wired to an EventLog. With hold=True, party(sid) blocks on a
    per-student gate until the test opens it (10 s failsafe so no thread is
    ever stranded)."""
    log = EventLog()
    gates = collections.defaultdict(threading.Event)

    def party(sid):
        log.record(f"party:{sid}")
        if hold:
            gates[sid].wait(10)

    room = Room(
        threshold=THRESHOLD,
        search=lambda: log.record("search"),
        breakup=lambda: log.record("breakup"),
        party=party,
    )
    return room, log, gates


def start_probe(fn):
    """Run fn on a daemon thread; the returned Event is set when it returns."""
    done = threading.Event()

    def body():
        fn()
        done.set()

    threading.Thread(target=body, daemon=True).start()
    return done


def wait_partying(runner, log, sids, timeout=5):
    """Wait until every student in sids is inside (its party event logged),
    failing fast if any worker raised."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        runner.raise_worker_errors()
        if all(log.count(f"party:{sid}") == 1 for sid in sids):
            return
        time.sleep(0.005)
    raise AssertionError(
        f"students {list(sids)} never all got into the room; log {log.events()}"
    )


def test_dean_searches_empty_room():
    room, log, _ = make_room(hold=False)
    assert_completes(
        room.dean_visit, timeout=2, msg="an empty room means search-and-go"
    )
    assert log.count("search") == 1, f"search() should have run once; log {log.events()}"
    assert log.count("breakup") == 0, "there was no party to break up"


def test_dean_waits_on_small_party():
    room, log, gates = make_room(hold=True)
    runner = ThreadRunner()
    for sid in (0, 1):
        runner.spawn(room.student_visit, sid, name=f"s{sid}")
    wait_partying(runner, log, (0, 1))
    dean = assert_blocks(
        room.dean_visit,
        msg="2 students: too many to search, too few to break up — the dean waits",
    )
    assert log.count("search") == 0 and log.count("breakup") == 0, (
        f"the dean acted on a room he may not enter; log {log.events()}"
    )
    gates[0].set()
    gates[1].set()
    assert dean.wait(5), "the room emptied — the waiting dean should search and go"
    assert log.count("search") == 1, (
        f"the dean entered an empty room: that's a search; log {log.events()}"
    )
    assert log.count("breakup") == 0, "there was never a big enough party to break up"
    runner.join_all(timeout=5)


def test_big_party_gets_broken_up():
    room, log, gates = make_room(hold=True)
    runner = ThreadRunner()
    threads = [
        runner.spawn(room.student_visit, sid, name=f"s{sid}") for sid in range(6)
    ]
    wait_partying(runner, log, range(6))
    dean = start_probe(room.dean_visit)
    eventually(
        lambda: log.count("breakup") == 1,
        timeout=5,
        msg="6 partiers with threshold 5: the dean must walk in and break it up",
    )
    assert log.count("search") == 0, "the dean searched a crowded room"
    # Two students try to slip in while the dean is inside.
    late = [start_probe(lambda s=s: room.student_visit(s)) for s in (10, 11)]
    time.sleep(0.3)
    assert log.count("party:10") == 0 and log.count("party:11") == 0, (
        f"a student entered while the dean was in the room; log {log.events()}"
    )
    # The partiers file out; the dean must stay until the last is gone.
    for sid in range(5):
        gates[sid].set()
    eventually(
        lambda: sum(1 for t in threads[:5] if not t.is_alive()) == 5,
        timeout=5,
        msg="students must be able to leave while the dean is in the room",
    )
    time.sleep(0.2)
    assert not dean.is_set(), "the dean left before the room was empty"
    assert log.count("party:10") == 0 and log.count("party:11") == 0, (
        f"a student slipped in mid-breakup; log {log.events()}"
    )
    gates[5].set()
    assert dean.wait(5), "the last student left — the dean should be done"
    # Only now may the two latecomers get in.
    gates[10].set()
    gates[11].set()
    for probe in late:
        assert probe.wait(5), "latecomers should get their party once the dean is gone"
    assert log.count("party:10") == 1 and log.count("party:11") == 1
    assert log.count("breakup") == 1 and log.count("search") == 0
    runner.join_all(timeout=5)


def test_students_may_leave_while_dean_inside():
    room, log, gates = make_room(hold=True)
    runner = ThreadRunner()
    threads = [
        runner.spawn(room.student_visit, sid, name=f"s{sid}") for sid in range(6)
    ]
    wait_partying(runner, log, range(6))
    dean = start_probe(room.dean_visit)
    eventually(
        lambda: log.count("breakup") == 1,
        timeout=5,
        msg="6 partiers with threshold 5: the dean must walk in and break it up",
    )
    for sid in range(5):
        gates[sid].set()
    eventually(
        lambda: sum(1 for t in threads[:5] if not t.is_alive()) == 5,
        timeout=5,
        msg="leaving must not be barred by the dean's presence",
    )
    time.sleep(0.2)
    assert not dean.is_set(), "the dean left with a student still inside"
    gates[5].set()
    assert dean.wait(5), "the dean should go once the last student is out"
    runner.join_all(timeout=5)


def test_growth_while_dean_waits():
    room, log, gates = make_room(hold=True)
    runner = ThreadRunner()
    for sid in (0, 1):
        runner.spawn(room.student_visit, sid, name=f"s{sid}")
    wait_partying(runner, log, (0, 1))
    dean = assert_blocks(
        room.dean_visit,
        msg="2 students: too many to search, too few to break up — the dean waits",
    )
    assert log.count("search") == 0 and log.count("breakup") == 0, (
        f"the dean acted on a room he may not enter; log {log.events()}"
    )
    # The party grows while the dean waits — he is not in the room, so
    # students walk right past him.
    for sid in (2, 3, 4, 5):
        runner.spawn(room.student_visit, sid, name=f"s{sid}")
    eventually(
        lambda: log.count("breakup") == 1,
        timeout=5,
        msg="the party outgrew the threshold — the waiting dean must storm in",
    )
    assert log.count("search") == 0, "the dean searched a room full of students"
    time.sleep(0.2)
    assert not dean.is_set(), "the dean left with students still inside"
    for sid in range(6):
        gates[sid].set()
    assert dean.wait(5), "everyone left — the dean should be done"
    runner.join_all(timeout=5)
    assert log.count("breakup") == 1 and log.count("search") == 0
