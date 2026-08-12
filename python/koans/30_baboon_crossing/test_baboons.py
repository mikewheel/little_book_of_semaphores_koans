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

from baboons import Rope


def _ops(rope, direction):
    if direction == "east":
        return rope.east_enter, rope.east_exit
    return rope.west_enter, rope.west_exit


def test_directions_never_mix():
    rope = Rope()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def baboon(direction, other):
        enter, exit_ = _ops(rope, direction)
        for _ in range(15):
            jitter()
            enter()
            snapshot = tracker.enter(direction)
            if snapshot.get(other, 0) > 0:
                tracker.violate(
                    f"an {direction}bound baboon got on while "
                    f"{snapshot[other]} {other}bound baboon(s) were on the rope"
                )
            jitter(1.0)
            tracker.exit(direction)
            exit_()

    for _ in range(6):
        runner.spawn(baboon, "east", "west")
        runner.spawn(baboon, "west", "east")
    runner.join_all(timeout=30)
    tracker.assert_no_violations()


def test_rope_holds_at_most_five():
    rope = Rope(capacity=5)
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def eastbound():
        for _ in range(8):
            rope.east_enter()
            with tracker.section("east"):
                jitter(1.5)
            rope.east_exit()

    for _ in range(8):
        runner.spawn(eastbound)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("east") <= 5, (
        f"the rope snapped: {tracker.max_concurrent('east')} baboons at once"
    )


def test_same_direction_shares():
    """One-baboon-at-a-time is not a crossing protocol; 5 must fit."""
    rope = Rope(capacity=5)
    tracker = OverlapTracker()
    all_on = threading.Event()
    runner = ThreadRunner()

    def eastbound():
        rope.east_enter()
        tracker.enter("east")
        all_on.wait(5)
        tracker.exit("east")
        rope.east_exit()

    for _ in range(5):
        runner.spawn(eastbound)
    eventually(
        lambda: runner.raise_worker_errors() or tracker.current("east") == 5,
        timeout=5,
        msg="5 same-direction baboons never shared the rope — over-serialized",
    )
    all_on.set()
    runner.join_all(timeout=5)


def test_capacity_frees_slots():
    """With 5 on the rope, #6 waits for an exit — not for an empty rope."""
    rope = Rope(capacity=5)
    for _ in range(5):
        assert_completes(rope.east_enter, timeout=2)
    probe = assert_blocks(
        rope.east_enter, msg="baboon #6 must wait while 5 are on the rope"
    )
    rope.east_exit()
    assert probe.wait(5), "one baboon stepped off — #6 should take its place"
    for _ in range(5):
        rope.east_exit()


def _fairness_trial():
    rope = Rope()
    log = EventLog()
    runner = ThreadRunner()

    # Two eastbound incumbents get on and stay (main thread lets them off).
    for _ in range(2):
        assert_completes(rope.east_enter, timeout=2)

    # A westbound baboon arrives and must wait its turn.
    def westbound():
        rope.west_enter()
        log.record("west_on")
        rope.west_exit()

    probe = assert_blocks(
        westbound, msg="a westbound baboon must wait while eastbound traffic owns the rope"
    )

    # Three more eastbound baboons arrive AFTER the westbound one.
    def late_east():
        rope.east_enter()
        log.record("late_east_on")
        rope.east_exit()

    for i in range(3):
        runner.spawn(late_east, name=f"late-east-{i}")
    time.sleep(0.25)
    runner.raise_worker_errors()
    assert log.count("late_east_on") == 0, (
        f"eastbound baboons that arrived after the waiting westbound one "
        f"overtook it: {log.events()}"
    )

    # Incumbents finish crossing; the westbound waiter goes next.
    for _ in range(2):
        rope.east_exit()
    assert probe.wait(5), "the waiting westbound baboon never crossed"
    log.wait_for_count("late_east_on", 3, timeout=5)
    log.assert_before("west_on", "late_east_on")
    runner.join_all(timeout=5)


def test_waiting_westbound_beats_late_eastbound():
    for _ in range(5):
        _fairness_trial()


def test_stress_mixed_traffic():
    rope = Rope()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def baboon(direction, other):
        enter, exit_ = _ops(rope, direction)
        for _ in range(10):
            jitter()
            enter()
            snapshot = tracker.enter(direction)
            if snapshot.get(other, 0) > 0:
                tracker.violate(f"{direction} and {other} on the rope together")
            if snapshot[direction] + snapshot.get(other, 0) > 5:
                tracker.violate(f"rope overloaded: {snapshot}")
            jitter(1.0)
            tracker.exit(direction)
            exit_()

    for _ in range(4):
        runner.spawn(baboon, "east", "west")
        runner.spawn(baboon, "west", "east")
    runner.join_all(timeout=30)
    tracker.assert_no_violations()
    assert tracker.max_combined() <= 5
