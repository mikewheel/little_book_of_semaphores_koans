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

from fair_bathroom import FairBathroom


def _ops(bathroom, gender):
    if gender == "male":
        return bathroom.male_enter, bathroom.male_exit
    return bathroom.female_enter, bathroom.female_exit


def test_genders_never_mix():
    bathroom = FairBathroom()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def person(gender, other):
        enter, exit_ = _ops(bathroom, gender)
        for _ in range(15):
            jitter()
            enter()
            snapshot = tracker.enter(gender)
            if snapshot.get(other, 0) > 0:
                tracker.violate(
                    f"a {gender} entered while {snapshot[other]} {other}(s) inside"
                )
            jitter(1.0)
            tracker.exit(gender)
            exit_()

    for _ in range(6):
        runner.spawn(person, "male", "female")
        runner.spawn(person, "female", "male")
    runner.join_all(timeout=30)
    tracker.assert_no_violations()


def test_capacity_respected():
    bathroom = FairBathroom(capacity=3)
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def male():
        for _ in range(10):
            bathroom.male_enter()
            with tracker.section("inside"):
                jitter(1.0)
            bathroom.male_exit()

    for _ in range(6):
        runner.spawn(male)
    runner.join_all(timeout=30)
    assert tracker.max_concurrent("inside") <= 3, (
        f"capacity 3 exceeded: saw {tracker.max_concurrent('inside')} inside"
    )


def test_same_gender_shares_up_to_capacity():
    """A room that admits one person at a time is safe but wrong."""
    bathroom = FairBathroom(capacity=3)
    tracker = OverlapTracker()
    all_in = threading.Event()
    runner = ThreadRunner()

    def female():
        bathroom.female_enter()
        tracker.enter("female")
        all_in.wait(5)
        tracker.exit("female")
        bathroom.female_exit()

    for _ in range(3):
        runner.spawn(female)
    eventually(
        lambda: runner.raise_worker_errors() or tracker.current("female") == 3,
        timeout=5,
        msg="3 women never shared the bathroom — the room is over-serialized",
    )
    all_in.set()
    runner.join_all(timeout=5)


def test_opposite_gender_blocks_until_empty():
    """A man enters only when the room is EMPTY, not when a slot frees up."""
    bathroom = FairBathroom()
    for _ in range(2):
        assert_completes(bathroom.female_enter, timeout=2)
    probe = assert_blocks(
        bathroom.male_enter,
        msg="a man must wait while women are inside",
    )
    bathroom.female_exit()
    assert not probe.wait(0.25), (
        "one woman left but another is still inside — the man must keep waiting"
    )
    bathroom.female_exit()
    assert probe.wait(5), "the room is empty — the waiting man should get in"
    bathroom.male_exit()


def _fairness_trial(waiter_gender, incumbent_gender):
    bathroom = FairBathroom()
    log = EventLog()
    runner = ThreadRunner()
    w_enter, w_exit = _ops(bathroom, waiter_gender)
    i_enter, i_exit = _ops(bathroom, incumbent_gender)

    # Two incumbents settle in and stay (main thread will let them out).
    for _ in range(2):
        assert_completes(i_enter, timeout=2)

    # The waiter arrives and must block: the room belongs to the others.
    def waiter():
        w_enter()
        log.record("waiter_in")
        w_exit()

    probe = assert_blocks(
        waiter,
        msg=f"the {waiter_gender} must wait while {incumbent_gender}s are inside",
    )

    # Three more of the incumbent gender arrive AFTER the waiter.
    def late_arrival():
        i_enter()
        log.record("late_in")
        i_exit()

    for i in range(3):
        runner.spawn(late_arrival, name=f"late-{i}")
    time.sleep(0.25)
    runner.raise_worker_errors()
    assert log.count("late_in") == 0, (
        f"{incumbent_gender}s who arrived after the waiting {waiter_gender} "
        f"slipped in ahead: {log.events()}"
    )

    # The incumbents leave; the waiter must be served before the latecomers.
    for _ in range(2):
        i_exit()
    assert probe.wait(5), f"the waiting {waiter_gender} never got in"
    log.wait_for_count("late_in", 3, timeout=5)
    log.assert_before("waiter_in", "late_in")
    runner.join_all(timeout=5)


def test_waiting_male_beats_late_females():
    for _ in range(5):
        _fairness_trial("male", "female")


def test_waiting_female_beats_late_males():
    for _ in range(5):
        _fairness_trial("female", "male")


def test_steady_stream_cannot_starve():
    """A continuous parade of women must not shut men out indefinitely."""
    bathroom = FairBathroom()
    stop = threading.Event()
    runner = ThreadRunner()

    def female_stream():
        while not stop.is_set():
            bathroom.female_enter()
            time.sleep(0.002)
            bathroom.female_exit()

    for _ in range(3):
        runner.spawn(female_stream)
    time.sleep(0.1)  # let the stream establish itself

    def male_visit():
        bathroom.male_enter()
        bathroom.male_exit()

    try:
        assert_completes(
            male_visit,
            timeout=4,
            msg="a man starved while women streamed through the bathroom",
        )
    finally:
        stop.set()
    runner.join_all(timeout=5)


def test_stress_mixed_traffic():
    bathroom = FairBathroom()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def person(gender, other):
        enter, exit_ = _ops(bathroom, gender)
        for _ in range(10):
            jitter()
            enter()
            snapshot = tracker.enter(gender)
            if snapshot.get(other, 0) > 0:
                tracker.violate(f"{gender} and {other} inside together")
            if snapshot[gender] + snapshot.get(other, 0) > 3:
                tracker.violate(f"over capacity: {snapshot}")
            jitter(1.0)
            tracker.exit(gender)
            exit_()

    for _ in range(4):
        runner.spawn(person, "male", "female")
        runner.spawn(person, "female", "male")
    runner.join_all(timeout=30)
    tracker.assert_no_violations()
    assert tracker.max_combined() <= 3
