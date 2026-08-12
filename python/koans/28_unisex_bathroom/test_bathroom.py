import threading

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from bathroom import Bathroom

CAPACITY = 3


def gendered_loop(bathroom, tracker, me, other, iterations=15):
    enter = bathroom.female_enter if me == "female" else bathroom.male_enter
    leave = bathroom.female_exit if me == "female" else bathroom.male_exit
    for _ in range(iterations):
        jitter()
        enter()
        snap = tracker.enter(me)
        if snap.get(other, 0):
            tracker.violate(
                f"a {me} entered while {snap[other]} {other}(s) were inside: {snap}"
            )
        if snap.get(me, 0) > CAPACITY:
            tracker.violate(f"more than {CAPACITY} {me}s inside: {snap}")
        jitter(1.0)
        tracker.exit(me)
        leave()


def test_genders_never_mix():
    bathroom = Bathroom(CAPACITY)
    tracker = OverlapTracker()
    runner = ThreadRunner()
    for _ in range(6):
        runner.spawn(gendered_loop, bathroom, tracker, "female", "male")
        runner.spawn(gendered_loop, bathroom, tracker, "male", "female")
    runner.join_all(timeout=20)
    tracker.assert_no_violations()


def test_capacity_respected():
    bathroom = Bathroom(CAPACITY)
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def worker():
        for _ in range(10):
            bathroom.female_enter()
            with tracker.section("female"):
                jitter(1.0)
            bathroom.female_exit()

    for _ in range(6):
        runner.spawn(worker)
    runner.join_all(timeout=20)
    assert tracker.max_concurrent("female") <= CAPACITY, (
        f"capacity {CAPACITY} exceeded: saw "
        f"{tracker.max_concurrent('female')} women inside at once"
    )


def test_same_gender_shares():
    bathroom = Bathroom(CAPACITY)
    tracker = OverlapTracker()
    release = threading.Event()
    runner = ThreadRunner()

    def lingerer():
        bathroom.female_enter()
        tracker.enter("female")
        release.wait(5)  # linger until all three are inside together
        tracker.exit("female")
        bathroom.female_exit()

    for _ in range(3):
        runner.spawn(lingerer)
    eventually(
        lambda: tracker.current("female") == 3,
        timeout=5,
        msg=f"only {tracker.current('female')} of 3 women made it inside "
        "together — same-gender sharing up to capacity is required",
    )
    release.set()
    runner.join_all(timeout=5)


def test_opposite_blocks_until_empty():
    bathroom = Bathroom(CAPACITY)
    assert_completes(bathroom.female_enter, timeout=2)
    assert_completes(bathroom.female_enter, timeout=2)

    probe = assert_blocks(
        bathroom.male_enter, msg="a man must wait while women are inside"
    )
    bathroom.female_exit()  # one woman leaves; the room is NOT empty yet
    assert not probe.wait(0.3), (
        "the man entered as soon as a slot freed — he may only enter once "
        "the bathroom is completely EMPTY"
    )
    bathroom.female_exit()  # the last woman leaves
    assert probe.wait(5), "the man should enter once the bathroom is empty"
    bathroom.male_exit()


def test_stress():
    bathroom = Bathroom(CAPACITY)
    tracker = OverlapTracker()
    runner = ThreadRunner()
    for _ in range(8):
        runner.spawn(gendered_loop, bathroom, tracker, "female", "male", 20)
        runner.spawn(gendered_loop, bathroom, tracker, "male", "female", 20)
    runner.join_all(timeout=30)
    tracker.assert_no_violations()
    assert tracker.max_combined() <= CAPACITY, (
        f"more than {CAPACITY} people inside at once: {tracker.max_combined()}"
    )
