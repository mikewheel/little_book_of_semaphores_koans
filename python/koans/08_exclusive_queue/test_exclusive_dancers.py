import threading
import time

from koan_utils import (
    EventLog,
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    jitter,
)

from exclusive_dancers import ExclusiveDanceFloor


def test_lone_leader_blocks():
    floor = ExclusiveDanceFloor()
    log = EventLog()
    probe = assert_blocks(
        lambda: floor.leader_dances(lambda: log.record("leader_danced")),
        msg="a leader with no follower must wait, not dance",
    )
    assert log.count("leader_danced") == 0, "the lone leader's dance ran anyway"
    threading.Thread(
        target=lambda: floor.follower_dances(lambda: log.record("follower_danced")),
        daemon=True,
    ).start()
    assert probe.wait(5), "the parked leader should dance once a follower arrives"
    assert log.count("leader_danced") == 1


def test_lone_follower_blocks():
    floor = ExclusiveDanceFloor()
    log = EventLog()
    probe = assert_blocks(
        lambda: floor.follower_dances(lambda: log.record("follower_danced")),
        msg="a follower with no leader must wait, not dance",
    )
    assert log.count("follower_danced") == 0, "the lone follower's dance ran anyway"
    threading.Thread(
        target=lambda: floor.leader_dances(lambda: log.record("leader_danced")),
        daemon=True,
    ).start()
    assert probe.wait(5), "the parked follower should dance once a leader arrives"
    assert log.count("follower_danced") == 1


def test_pair_dances_together():
    floor = ExclusiveDanceFloor()
    tracker = OverlapTracker()
    log = EventLog()
    runner = ThreadRunner()

    def dance(kind, other):
        tracker.enter(f"{kind}_dance")
        try:
            log.record(f"enter:{kind}")
            # Stay on the floor until the partner has demonstrably joined it.
            log.wait_for_count(f"enter:{other}", 1, timeout=5)
        finally:
            tracker.exit(f"{kind}_dance")

    runner.spawn(
        lambda: floor.leader_dances(lambda: dance("leader", "follower")),
        name="leader",
    )
    runner.spawn(
        lambda: floor.follower_dances(lambda: dance("follower", "leader")),
        name="follower",
    )
    runner.join_all(timeout=10)
    assert tracker.max_combined() == 2, (
        "the pair's two dance callbacks never overlapped — partners must "
        "dance together"
    )


def test_leader_returns_only_after_partners_dance_ends():
    floor = ExclusiveDanceFloor()
    for _ in range(2):
        log = EventLog()
        runner = ThreadRunner()

        def follower():
            def dance():
                log.record("follower_start")
                time.sleep(0.15)
                log.record("follower_end")

            floor.follower_dances(dance)

        runner.spawn(follower, name="follower")
        time.sleep(0.05)  # let the follower reach the floor and wait
        assert_completes(
            lambda: floor.leader_dances(lambda: log.record("leader_danced")),
            timeout=5,
        )
        assert log.count("follower_end") == 1, (
            "leader_dances() returned while its partner was still dancing"
        )
        runner.join_all(timeout=5)


def test_parked_leader_still_waits_for_its_partner():
    floor = ExclusiveDanceFloor()
    log = EventLog()
    probe = assert_blocks(
        lambda: floor.leader_dances(lambda: log.record("leader_danced")),
        msg="a lone leader must wait for a partner",
    )
    runner = ThreadRunner()

    def follower():
        def dance():
            log.record("follower_start")
            time.sleep(0.15)
            log.record("follower_end")

        floor.follower_dances(dance)

    runner.spawn(follower, name="follower")
    assert probe.wait(5), "the parked leader should return once the pair danced"
    assert log.count("follower_end") == 1, (
        "the leader returned before its partner finished dancing"
    )
    runner.join_all(timeout=5)


def test_at_most_one_pair_on_floor():
    floor = ExclusiveDanceFloor()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def dancer(method, category):
        jitter(3.0)

        def dance():
            with tracker.section(category, dwell=0.01):
                jitter(1.0)

        method(dance)

    for _ in range(6):
        runner.spawn(dancer, floor.leader_dances, "leader_dance")
        runner.spawn(dancer, floor.follower_dances, "follower_dance")
    runner.join_all(timeout=15)
    assert tracker.max_concurrent("leader_dance") == 1, (
        f"{tracker.max_concurrent('leader_dance')} leaders danced at once"
    )
    assert tracker.max_concurrent("follower_dance") == 1, (
        f"{tracker.max_concurrent('follower_dance')} followers danced at once"
    )
    assert tracker.max_combined() <= 2, (
        f"{tracker.max_combined()} dancers shared the floor — that is more "
        "than one pair"
    )


def test_all_complete_stress():
    floor = ExclusiveDanceFloor()
    done = EventLog()
    runner = ThreadRunner()

    def dancer(kind, method):
        jitter(3.0)
        method(lambda: done.record(f"{kind}_danced"))
        done.record(f"{kind}_returned")

    for _ in range(20):
        runner.spawn(dancer, "leader", floor.leader_dances)
        runner.spawn(dancer, "follower", floor.follower_dances)
    runner.join_all(timeout=20)
    assert done.count("leader_danced") == 20
    assert done.count("follower_danced") == 20
    assert done.count("leader_returned") == 20
    assert done.count("follower_returned") == 20
