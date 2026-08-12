import random
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

from modus_hall import Path


def test_factions_never_mix():
    path = Path()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def member(faction, other):
        crosser = path.heathen_cross if faction == "heathen" else path.prude_cross

        def cross():
            snapshot = tracker.enter(faction)
            if snapshot.get(other, 0) > 0:
                tracker.violate(
                    f"a {faction} was on the path with {snapshot[other]} {other}(s)"
                )
            time.sleep(random.uniform(0.001, 0.003))
            tracker.exit(faction)

        for _ in range(10):
            jitter()
            crosser(cross)

    for _ in range(8):
        runner.spawn(member, "heathen", "prude")
        runner.spawn(member, "prude", "heathen")
    runner.join_all(timeout=30)
    tracker.assert_no_violations()


def test_first_arrival_claims_field():
    path = Path()
    log = EventLog()
    assert_completes(
        lambda: path.heathen_cross(lambda: log.record("crossed")),
        timeout=2,
        msg="a lone heathen on an empty path must cross immediately",
    )
    assert log.count("crossed") == 1


def test_same_faction_shares():
    path = Path()
    tracker = OverlapTracker()
    all_on = threading.Event()
    runner = ThreadRunner()

    def heathen():
        def cross():
            tracker.enter("heathen")
            all_on.wait(5)
            tracker.exit("heathen")

        path.heathen_cross(cross)

    for _ in range(4):
        runner.spawn(heathen)
    eventually(
        lambda: runner.raise_worker_errors() or tracker.current("heathen") == 4,
        timeout=5,
        msg="4 heathens never shared the path — the same faction must not "
        "block itself",
    )
    all_on.set()
    runner.join_all(timeout=5)


def test_majority_flips_the_field():
    for _ in range(5):
        _majority_trial()


def _majority_trial():
    path = Path()
    log = EventLog()
    runner = ThreadRunner()
    hold = threading.Event()

    # Two heathens take the path and stay on it.
    def incumbent():
        def cross():
            log.record("heathen_on_path")
            hold.wait(10)

        path.heathen_cross(cross)

    runner.spawn(incumbent)
    runner.spawn(incumbent)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("heathen_on_path") == 2,
        timeout=5,
        msg="two heathens never took the empty path",
    )

    # Three prudes arrive: 3 waiting prudes > 2 crossing heathens — the
    # balance tips to the prudes.
    def prude():
        log.record("prude_arrived")
        path.prude_cross(lambda: log.record("prude_on_path"))

    for i in range(3):
        runner.spawn(prude, name=f"prude-{i}")
    eventually(
        lambda: runner.raise_worker_errors() or log.count("prude_arrived") == 3,
        timeout=5,
    )
    time.sleep(0.3)  # let all three check in and tip the balance
    assert log.count("prude_on_path") == 0, (
        "prudes crossed while heathens were still on the path"
    )

    # A heathen arriving after the tip must NOT cross before the prude batch.
    late_probe = assert_blocks(
        lambda: path.heathen_cross(lambda: log.record("late_heathen_on_path")),
        msg="a heathen arriving after the prudes gained majority must queue",
    )

    # Incumbents finish: the whole prude cohort crosses, then the heathen.
    hold.set()
    log.wait_for_count("prude_on_path", 3, timeout=5)
    assert late_probe.wait(5), "the late heathen never got to cross"
    log.assert_before("prude_on_path", "late_heathen_on_path")
    runner.join_all(timeout=10)


def test_minority_keeps_waiting():
    path = Path()
    log = EventLog()
    runner = ThreadRunner()
    hold = threading.Event()

    # Three heathens hold the path.
    def incumbent():
        def cross():
            log.record("heathen_on_path")
            hold.wait(10)

        path.heathen_cross(cross)

    for _ in range(3):
        runner.spawn(incumbent)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("heathen_on_path") == 3,
        timeout=5,
    )

    # Two prudes arrive: 2 < 3, no majority — they must wait.
    def prude():
        log.record("prude_arrived")
        path.prude_cross(lambda: log.record("prude_on_path"))

    for _ in range(2):
        runner.spawn(prude)
    eventually(
        lambda: runner.raise_worker_errors() or log.count("prude_arrived") == 2,
        timeout=5,
    )
    time.sleep(0.25)
    assert log.count("prude_on_path") == 0, "an outnumbered prude crossed"

    # Heathen #4 strolls through: his faction still rules.
    assert_completes(
        lambda: path.heathen_cross(lambda: log.record("h4_on_path")),
        timeout=2,
        msg="a heathen must pass freely while his faction holds the path "
        "and the opposition lacks a majority",
    )
    assert log.count("prude_on_path") == 0, (
        "prudes crossed while still outnumbered"
    )

    # Cleanup: incumbents leave; the prude pair finally gets the path.
    hold.set()
    log.wait_for_count("prude_on_path", 2, timeout=5)
    runner.join_all(timeout=10)


def test_everyone_eventually_crosses():
    path = Path()
    log = EventLog()
    runner = ThreadRunner()

    def member(faction):
        crosser = path.heathen_cross if faction == "h" else path.prude_cross
        for _ in range(5):
            jitter()
            crosser(lambda: (log.record(faction), jitter(1.0)))

    for _ in range(8):
        runner.spawn(member, "h")
        runner.spawn(member, "p")
    runner.join_all(timeout=30)
    assert log.count("h") == 40, f"only {log.count('h')} heathen crossings"
    assert log.count("p") == 40, f"only {log.count('p')} prude crossings"
