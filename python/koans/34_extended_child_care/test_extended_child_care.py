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

from extended_child_care import ExtendedChildCare

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


# ── Everything koan 33 demanded still holds ──────────────────────────────


def test_ratio_never_violated():
    """Stress: adults and children churn; the bound holds at every sample.

    Bookkeeping order avoids false alarms: adults are counted from just
    before adult_enter until just after adult_leave returns (over-count,
    which also matches the rule that a waiting leaver is still inside),
    children only between child_enter returning and child_leave being
    called (under-count). Real violations still show up.
    """
    cc = ExtendedChildCare(RATIO)
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
    cc = ExtendedChildCare(RATIO)
    probe = assert_blocks(
        cc.child_enter, msg="with no adult inside, a child must wait at the door"
    )
    assert_completes(cc.adult_enter, timeout=2, msg="adult_enter must never block")
    assert probe.wait(5), "the waiting child should be admitted once an adult arrives"


def test_adult_admits_three():
    cc = ExtendedChildCare(RATIO)
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
    cc = ExtendedChildCare(RATIO)
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
    """Koan 33's nemesis schedule must still work here.

    2 adults, 6 children; both adults call adult_leave; children trickle
    out until exactly one departure is legal. Exactly one leaver gets out.
    """
    for trial in range(3):
        cc = ExtendedChildCare(RATIO)
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


# ── The new requirement: no unnecessary waiting ──────────────────────────


def test_child_admitted_despite_waiting_adult():
    """THE extended-child-care scenario.

    2 adults + 4 children inside; one adult gets stuck trying to leave
    (4 > 3x1). A 5th child arrives. 5 <= 3x2 — the stuck adult is still
    inside, so the child MUST be admitted promptly. A solution where the
    leaver has already pocketed part of the capacity turns the child away.
    """
    cc = ExtendedChildCare(RATIO)
    cc.adult_enter()
    cc.adult_enter()
    for i in range(4):
        assert_completes(
            cc.child_enter, timeout=2, msg=f"child {i + 1} of 4 fits under 2 adults"
        )
    leaver = assert_blocks(
        cc.adult_leave, msg="an adult may not leave 4 children with one adult"
    )
    assert_completes(
        cc.child_enter,
        timeout=2,
        msg="a 5th child fits (5 <= 3 x 2): the adult stuck at the door "
        "must not bar the way in",
    )
    time.sleep(0.2)
    assert not leaver.is_set(), (
        "the adult left while 5 children needed both adults"
    )
    for _ in range(3):
        cc.child_leave()
    assert leaver.wait(5), (
        "2 children remain (2 <= 3x1) — the waiting adult should now be out"
    )


def test_waiting_adult_leaves_when_kids_drop():
    """Continue the scenario: a stuck leaver departs the moment it can."""
    cc = ExtendedChildCare(RATIO)
    cc.adult_enter()
    cc.adult_enter()
    for i in range(5):
        assert_completes(
            cc.child_enter, timeout=2, msg=f"child {i + 1} of 5 fits under 2 adults"
        )
    leaver = assert_blocks(
        cc.adult_leave, msg="an adult may not leave 5 children with one adult"
    )
    cc.child_leave()  # 4 children: still too many for one adult
    time.sleep(0.2)
    assert not leaver.is_set(), "the adult left while 4 children needed both"
    cc.child_leave()  # 3 children: 3 <= 3x1 — departure is now legal
    assert leaver.wait(5), (
        "3 children fit under the remaining adult — the waiting adult "
        "should leave immediately"
    )
    second = assert_blocks(
        cc.adult_leave, msg="the last adult may not abandon 3 children"
    )
    for _ in range(3):
        cc.child_leave()
    assert second.wait(5), "the center is empty — the last adult should get out"
