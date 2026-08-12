import threading
import time

from koan_utils import (
    EventLog,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from senate_bus import BusStop

CAPACITY = 5


def make_stop(gate=None):
    """A BusStop wired to an EventLog.

    The board hook logs board_begin:rid, optionally holds on `gate` (a
    threading.Event the test opens; 10 s failsafe), then logs board:rid.
    The depart hook logs depart:n.
    """
    log = EventLog()

    def board(rid):
        log.record(f"board_begin:{rid}")
        if gate is not None:
            gate.wait(10)
        log.record(f"board:{rid}")

    def depart(n):
        log.record(f"depart:{n}")

    stop = BusStop(capacity=CAPACITY, board=board, depart=depart)
    return stop, log


def arrive(stop, log, rid):
    """Rider wrapper: logs the walk-up so tests can see arrivals even when
    rider() itself blocks."""
    log.record(f"at_stop:{rid}")
    stop.rider(rid)


def start_probe(fn):
    """Run fn on a daemon thread; the returned Event is set when it returns."""
    done = threading.Event()

    def body():
        fn()
        done.set()

    threading.Thread(target=body, daemon=True).start()
    return done


def boarders(log):
    return [e.split(":", 1)[1] for e in log.events() if e.startswith("board:")]


def departs(log):
    return [int(e.split(":", 1)[1]) for e in log.events() if e.startswith("depart:")]


def test_empty_stop_departs_immediately():
    stop, log = make_stop()
    assert_completes(
        stop.bus_arrives, timeout=2, msg="a bus at an empty stop departs at once"
    )
    assert log.events() == ["depart:0"], (
        f"expected exactly depart(0) and no boarding; log {log.events()}"
    )
    # A rider who shows up after that bus is gone must wait for the next one.
    probe = assert_blocks(
        lambda: arrive(stop, log, 1),
        msg="a rider who missed the bus waits at the stop",
    )
    assert_completes(stop.bus_arrives, timeout=2, msg="the next bus visit")
    assert probe.wait(5), "the next bus should pick up the waiting rider"
    assert log.count("board:1") == 1 and log.count("depart:1") == 1, (
        f"the second bus should take exactly rider 1; log {log.events()}"
    )


def test_waiting_riders_board():
    stop, log = make_stop()
    runner = ThreadRunner()
    for rid in range(3):
        runner.spawn(arrive, stop, log, rid, name=f"r{rid}")
    for rid in range(3):
        log.wait_for_count(f"at_stop:{rid}", 1, timeout=5)
    time.sleep(0.25)  # let the walk-ups actually join the waiting queue
    assert_completes(
        stop.bus_arrives, timeout=5, msg="a bus serving 3 waiting riders"
    )
    runner.join_all(timeout=5)
    assert departs(log) == [3], (
        f"one bus, three riders: expected depart(3); log {log.events()}"
    )
    assert sorted(boarders(log)) == ["0", "1", "2"], (
        f"all three waiting riders (and nobody else) board; log {log.events()}"
    )


def test_overflow_waits_for_next_bus():
    stop, log = make_stop()
    probes = [start_probe(lambda r=r: arrive(stop, log, r)) for r in range(8)]
    for rid in range(8):
        log.wait_for_count(f"at_stop:{rid}", 1, timeout=5)
    time.sleep(0.25)  # let the walk-ups actually join the waiting queue
    assert_completes(
        stop.bus_arrives, timeout=5, msg="the first bus (5 of 8 riders fit)"
    )
    assert departs(log) == [CAPACITY], (
        f"8 waiting, capacity 5: the first bus departs with 5; log {log.events()}"
    )
    eventually(
        lambda: sum(p.is_set() for p in probes) == CAPACITY,
        timeout=5,
        msg="the 5 boarded riders should be done",
    )
    time.sleep(0.2)
    assert sum(p.is_set() for p in probes) == CAPACITY, (
        "more riders than the bus's capacity got aboard"
    )
    assert_completes(
        stop.bus_arrives, timeout=5, msg="the second bus (the 3 left behind)"
    )
    for probe in probes:
        assert probe.wait(5), "everyone should be served after two buses"
    assert departs(log) == [5, 3], f"expected depart(5) then depart(3); log {log.events()}"
    assert sorted(boarders(log)) == sorted(str(r) for r in range(8)), (
        f"every rider boards exactly once; log {log.events()}"
    )


def test_late_arrivals_wait():
    """Riders who walk up mid-boarding take the NEXT bus, never this one."""
    for trial in range(5):
        gate = threading.Event()
        stop, log = make_stop(gate=gate)
        early = [start_probe(lambda r=r: arrive(stop, log, r)) for r in (1, 2)]
        for rid in (1, 2):
            log.wait_for_count(f"at_stop:{rid}", 1, timeout=5)
        time.sleep(0.2)  # let both join the waiting queue
        bus = start_probe(stop.bus_arrives)
        eventually(
            lambda: any(e.startswith("board_begin:") for e in log.events()),
            timeout=5,
            msg=f"trial {trial}: boarding should have begun",
        )
        # Boarding is now frozen mid-step; two more riders walk up.
        late = [start_probe(lambda r=r: arrive(stop, log, r)) for r in (3, 4)]
        for rid in (3, 4):
            log.wait_for_count(f"at_stop:{rid}", 1, timeout=5)
        time.sleep(0.2)  # give them every chance to (wrongly) sneak aboard
        gate.set()
        assert bus.wait(5), f"trial {trial}: the bus never departed"
        assert departs(log) == [2], (
            f"trial {trial}: only the 2 riders present at arrival board this "
            f"bus; log {log.events()}"
        )
        assert sorted(boarders(log)) == ["1", "2"], (
            f"trial {trial}: a mid-boarding walk-up boarded the current bus; "
            f"log {log.events()}"
        )
        for probe in early:
            assert probe.wait(5), f"trial {trial}: an early rider never boarded"
        time.sleep(0.2)
        assert not any(p.is_set() for p in late), (
            f"trial {trial}: a late rider finished without a second bus"
        )
        assert_completes(
            stop.bus_arrives, timeout=5, msg="the next bus (for the late riders)"
        )
        for probe in late:
            assert probe.wait(5), (
                f"trial {trial}: the next bus should take the late riders"
            )
        assert departs(log) == [2, 2], (
            f"trial {trial}: each bus takes its own pair; log {log.events()}"
        )
        assert sorted(boarders(log)) == ["1", "2", "3", "4"]


def test_stress():
    stop, log = make_stop()
    runner = ThreadRunner()

    def rider(rid):
        jitter(5.0)
        arrive(stop, log, rid)

    for rid in range(30):
        runner.spawn(rider, rid, name=f"r{rid}")
    deadline = time.monotonic() + 15
    while len(boarders(log)) < 30:
        assert time.monotonic() < deadline, (
            f"the fleet never served all 30 riders; log has "
            f"{len(boarders(log))} boardings"
        )
        runner.raise_worker_errors()
        assert_completes(stop.bus_arrives, timeout=5, msg="a bus visit")
        jitter(3.0)
    runner.join_all(timeout=5)
    assert all(n <= CAPACITY for n in departs(log)), (
        f"some bus departed over capacity: {departs(log)}"
    )
    assert sum(departs(log)) == 30, (
        f"depart() counts must add up to the 30 riders served: {departs(log)}"
    )
    assert sorted(boarders(log)) == sorted(str(r) for r in range(30)), (
        "every rider boards exactly once"
    )
