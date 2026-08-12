import threading
import time

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from child_care import ChildCare

RATIO = 3


def snapshot(tracker):
    """Atomic occupancy snapshot (the probe category is transient)."""
    snap = tracker.enter("probe")
    tracker.exit("probe")
    return snap


def check_ratio(tracker, snap, where):
    children = snap.get("children", 0)
    adults = snap.get("adults", 0)
    if children > RATIO * adults:
        tracker.violate(
            f"{where}: {children} children with only {adults} adult(s) inside"
        )


def test_ratio_never_violated():
    """Stress: adults and children churn; the bound holds at every sample.

    Bookkeeping order avoids false alarms: adults are counted from just
    before adult_enter until just after adult_leave returns (over-count),
    children only between child_enter returning and child_leave being
    called (under-count). Real violations still show up.
    """
    cc = ChildCare(RATIO)
    tracker = OverlapTracker()
    runner = ThreadRunner()
    children_done = threading.Event()

    def adult():
        while not children_done.is_set():
            tracker.enter("adults")
            cc.adult_enter()
            jitter(3.0)
            cc.adult_leave()
            tracker.exit("adults")
            jitter(1.0)

    def child():
        for _ in range(8):
            cc.child_enter()
            check_ratio(tracker, tracker.enter("children"), "on child entry")
            jitter(2.0)
            tracker.exit("children")
            cc.child_leave()
            jitter(1.0)

    for i in range(4):
        runner.spawn(adult, name=f"adult{i}")
    child_threads = [runner.spawn(child, name=f"child{i}") for i in range(10)]

    deadline = time.monotonic() + 25
    while any(t.is_alive() for t in child_threads):
        assert time.monotonic() < deadline, (
            "the children never finished their visits (deadlock?)"
        )
        runner.raise_worker_errors()
        check_ratio(tracker, snapshot(tracker), "sampled")
        time.sleep(0.002)
    children_done.set()
    runner.join_all(timeout=10)
    tracker.assert_no_violations()


def test_child_blocks_without_adults():
    cc = ChildCare(RATIO)
    probe = assert_blocks(
        cc.child_enter, msg="with no adult inside, a child must wait at the door"
    )
    assert_completes(cc.adult_enter, timeout=2, msg="adult_enter must never block")
    assert probe.wait(5), "the waiting child should be admitted once an adult arrives"


def test_adult_admits_three():
    cc = ChildCare(RATIO)
    assert_completes(cc.adult_enter, timeout=2, msg="adult_enter must never block")
    for i in range(RATIO):
        assert_completes(
            cc.child_enter,
            timeout=2,
            msg=f"child {i + 1} of {RATIO} fits under one adult and must get in",
        )
    probe = assert_blocks(
        cc.child_enter,
        msg=f"child {RATIO + 1} must wait: one adult supervises at most {RATIO}",
    )
    cc.adult_enter()
    assert probe.wait(5), "a second adult arrived — the waiting child should get in"


def test_adult_leave_blocks_while_needed():
    cc = ChildCare(RATIO)
    cc.adult_enter()
    cc.child_enter()
    cc.child_enter()
    probe = assert_blocks(
        cc.adult_leave, msg="the only adult may not abandon 2 children"
    )
    assert_completes(cc.child_leave, timeout=2, msg="child_leave must never block")
    time.sleep(0.15)
    assert not probe.is_set(), (
        "the adult left while 1 child was still inside with nobody else"
    )
    cc.child_leave()
    assert probe.wait(5), "the room emptied — the adult should be free to go"


def test_two_adults_leaving_no_deadlock():
    """The book's schedule: two adults head for the door at the same time.

    Room state: 2 adults, 6 children (capacity exactly full). Both adults
    call adult_leave; children then trickle out one by one until exactly one
    departure is legal. Exactly one leaver must get out — a solution that
    lets the leavers split the freed capacity deadlocks both.
    """
    for trial in range(3):
        cc = ChildCare(RATIO)
        cc.adult_enter()
        cc.adult_enter()
        for i in range(2 * RATIO):
            assert_completes(
                cc.child_enter, timeout=2, msg=f"child {i + 1} of 6 fits under 2 adults"
            )
        first = assert_blocks(
            cc.adult_leave, msg="no adult may leave while 6 children need both"
        )
        second = assert_blocks(
            cc.adult_leave, msg="no adult may leave while 6 children need both"
        )
        for _ in range(RATIO):
            time.sleep(0.05)
            cc.child_leave()
        eventually(
            lambda: first.is_set() or second.is_set(),
            timeout=2,
            msg=f"trial {trial}: 3 children remain, so one adult can leave — "
            "but neither did (did the leavers deadlock each other?)",
        )
        time.sleep(0.3)
        left = int(first.is_set()) + int(second.is_set())
        assert left == 1, (
            f"trial {trial}: exactly one adult may leave 3 children with one "
            f"adult, but {left} got out"
        )
        for _ in range(RATIO):
            cc.child_leave()
        remaining = second if first.is_set() else first
        assert remaining.wait(5), (
            f"trial {trial}: the center is empty — the second adult should get out"
        )
