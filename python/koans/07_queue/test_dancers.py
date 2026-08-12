import threading
import time

from koan_utils import EventLog, ThreadRunner, assert_blocks, eventually, jitter

from dancers import DanceFloor


def test_lone_leader_blocks():
    floor = DanceFloor()
    probe = assert_blocks(
        floor.leader_arrives, msg="a leader with no follower must wait"
    )
    threading.Thread(target=floor.follower_arrives, daemon=True).start()
    assert probe.wait(5), "the parked leader should proceed once a follower arrives"


def test_lone_follower_blocks():
    floor = DanceFloor()
    probe = assert_blocks(
        floor.follower_arrives, msg="a follower with no leader must wait"
    )
    threading.Thread(target=floor.leader_arrives, daemon=True).start()
    assert probe.wait(5), "the parked follower should proceed once a leader arrives"


def test_pair_completes():
    for _ in range(20):
        floor = DanceFloor()
        runner = ThreadRunner()
        runner.spawn(floor.leader_arrives, name="leader")
        runner.spawn(floor.follower_arrives, name="follower")
        runner.join_all(timeout=5)


def test_three_leaders_one_follower():
    floor = DanceFloor()
    done = EventLog()
    runner = ThreadRunner()

    def leader():
        floor.leader_arrives()
        done.record("leader")

    def follower():
        floor.follower_arrives()
        done.record("follower")

    for _ in range(3):
        runner.spawn(leader)
    time.sleep(0.2)
    runner.raise_worker_errors()
    assert done.count("leader") == 0, "no leader may proceed before any follower arrives"

    runner.spawn(follower)
    eventually(
        lambda: done.count("leader") == 1 and done.count("follower") == 1,
        timeout=5,
        msg="one follower should release exactly one pair",
    )
    time.sleep(0.3)  # a window for surplus leaders to sneak out
    assert done.count("leader") == 1, (
        f"one follower released {done.count('leader')} leaders"
    )

    runner.spawn(follower)
    runner.spawn(follower)
    runner.join_all(timeout=5)
    assert done.count("leader") == 3
    assert done.count("follower") == 3


def test_balanced_stress():
    floor = DanceFloor()
    done = EventLog()
    runner = ThreadRunner()

    def dancer(kind, arrive):
        jitter(3.0)
        arrive()
        done.record(kind)

    for _ in range(30):
        runner.spawn(dancer, "leader", floor.leader_arrives)
        runner.spawn(dancer, "follower", floor.follower_arrives)
    runner.join_all(timeout=15)
    assert done.count("leader") == 30
    assert done.count("follower") == 30
