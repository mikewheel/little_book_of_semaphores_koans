import threading
import time
import types

from koan_utils import EventLog, ThreadRunner, assert_completes, jitter

from faneuil_hall import FaneuilHall


def make_hall(log, gates=None):
    """Build a hall whose hooks record into ``log``.

    ``gates`` maps an event label to a threading.Event; a gated hook records
    ``<label>:pending`` and then blocks until its gate is set, only recording
    the bare label once released. The 20 s cap is a safety net so an
    orphaned hook can never outlive the test by much.
    """
    gates = gates or {}

    def fire(label):
        gate = gates.get(label)
        if gate is not None:
            log.record(label + ":pending")
            gate.wait(20)
        log.record(label)

    hooks = types.SimpleNamespace(
        enter=lambda who: fire(f"enter:{who}"),
        check_in=lambda iid: fire(f"check_in:{iid}"),
        sit_down=lambda iid: fire(f"sit_down:{iid}"),
        swear=lambda iid: fire(f"swear:{iid}"),
        get_certificate=lambda iid: fire(f"certificate:{iid}"),
        confirm=lambda: fire("confirm"),
        spectate=lambda sid: fire(f"spectate:{sid}"),
        leave=lambda who: fire(f"leave:{who}"),
    )
    return FaneuilHall(hooks)


def release_all(gates):
    for gate in gates.values():
        gate.set()


def wait_logged(runner, log, label, n=1, timeout=5.0):
    """Wait for the nth ``label`` event, failing fast if a worker raised."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        runner.raise_worker_errors()
        if log.count(label) >= n:
            return
        time.sleep(0.005)
    raise AssertionError(f"timed out waiting for {n} x {label!r}; log was {log.events()}")


def test_ceremony_happy_path():
    log = EventLog()
    hall = make_hall(log)
    runner = ThreadRunner()
    for i in (1, 2, 3):
        runner.spawn(hall.immigrant, i, name=f"imm{i}")
    for i in (1, 2, 3):
        wait_logged(runner, log, f"check_in:{i}")
    runner.spawn(hall.judge_visit, name="judge")
    runner.join_all(timeout=10)
    assert log.count("confirm") == 1
    for i in (1, 2, 3):
        log.assert_before(f"check_in:{i}", "confirm")
        log.assert_before("confirm", f"certificate:{i}")
        log.assert_before(f"certificate:{i}", f"leave:immigrant:{i}")
        log.assert_before("leave:judge", f"leave:immigrant:{i}")


def test_judge_waits_for_checkins():
    log = EventLog()
    gates = {"check_in:2": threading.Event()}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        for i in (1, 3):
            runner.spawn(hall.immigrant, i, name=f"imm{i}")
        wait_logged(runner, log, "check_in:1")
        wait_logged(runner, log, "check_in:3")
        runner.spawn(hall.immigrant, 2, name="imm2")  # the slow one
        wait_logged(runner, log, "check_in:2:pending")  # 2 is mid-check-in
        runner.spawn(hall.judge_visit, name="judge")
        time.sleep(0.3)
        runner.raise_worker_errors()
        assert log.count("confirm") == 0, (
            "the judge confirmed before every immigrant who entered had "
            f"checked in; log was {log.events()}"
        )
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    log.assert_before("check_in:2", "confirm")
    assert log.count("confirm") == 1


def test_door_locked_while_judge_inside():
    log = EventLog()
    gates = {"confirm": threading.Event()}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall.judge_visit, name="judge")
        wait_logged(runner, log, "confirm:pending")  # judge is mid-ceremony
        runner.spawn(hall.immigrant, 9, name="imm9")
        runner.spawn(hall.spectator, 7, name="spec7")
        time.sleep(0.3)
        runner.raise_worker_errors()
        assert log.count("enter:immigrant:9") == 0, (
            "an immigrant entered while the judge was in the building"
        )
        assert log.count("enter:spectator:7") == 0, (
            "a spectator entered while the judge was in the building"
        )
    finally:
        release_all(gates)
    # Once the judge leaves, both walk in.
    wait_logged(runner, log, "enter:immigrant:9")
    wait_logged(runner, log, "enter:spectator:7")
    log.assert_before("leave:judge", "enter:immigrant:9")
    log.assert_before("leave:judge", "enter:spectator:7")
    # A second visit swears in the latecomer so everyone can finish.
    wait_logged(runner, log, "check_in:9")
    runner.spawn(hall.judge_visit, name="judge2")
    runner.join_all(timeout=10)
    assert log.count("confirm") == 2


def test_immigrants_cannot_leave_with_judge_inside():
    log = EventLog()
    gates = {"leave:judge": threading.Event()}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall.immigrant, 1, name="imm1")
        wait_logged(runner, log, "check_in:1")
        runner.spawn(hall.judge_visit, name="judge")
        # The immigrant is sworn in and holds her certificate, but the
        # judge is still in the building (held mid-leave by the gate).
        wait_logged(runner, log, "certificate:1")
        wait_logged(runner, log, "leave:judge:pending")
        time.sleep(0.3)
        runner.raise_worker_errors()
        assert log.count("leave:immigrant:1") == 0, (
            "an immigrant left while the judge was in the building"
        )
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    log.assert_before("leave:judge", "leave:immigrant:1")


def test_spectators_unaffected():
    # With no judge around, a spectator sails straight through.
    log = EventLog()
    hall = make_hall(log)
    assert_completes(lambda: hall.spectator(1), timeout=5)
    for label in ("enter:spectator:1", "spectate:1", "leave:spectator:1"):
        assert log.count(label) == 1
    # A spectator already inside may leave while the judge is present.
    log2 = EventLog()
    gates = {"confirm": threading.Event(), "spectate:2": threading.Event()}
    hall2 = make_hall(log2, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall2.spectator, 2, name="spec2")
        wait_logged(runner, log2, "spectate:2:pending")  # she's watching
        runner.spawn(hall2.judge_visit, name="judge")
        wait_logged(runner, log2, "confirm:pending")  # judge is inside
        gates["spectate:2"].set()  # the show is over for her
        wait_logged(runner, log2, "leave:spectator:2")
        assert log2.count("confirm") == 0, (
            "expected the spectator to leave while the judge was still "
            "mid-visit — spectators must be free to go"
        )
    finally:
        release_all(gates)
    runner.join_all(timeout=10)


def test_two_ceremonies():
    log = EventLog()
    hall = make_hall(log)
    runner = ThreadRunner()
    for batch, spectator in (((1, 2), 51), ((3, 4, 5), 52)):
        for i in batch:
            runner.spawn(lambda i=i: (jitter(), hall.immigrant(i)), name=f"imm{i}")
        runner.spawn(lambda s=spectator: (jitter(), hall.spectator(s)), name=f"spec{spectator}")
        for i in batch:
            wait_logged(runner, log, f"check_in:{i}")
        runner.spawn(hall.judge_visit, name="judge")
        runner.join_all(timeout=10)
    assert log.count("confirm") == 2
    assert log.count("enter:judge") == 2
    assert log.count("leave:judge") == 2
    for i in (1, 2, 3, 4, 5):
        assert log.count(f"check_in:{i}") == 1
        assert log.count(f"certificate:{i}") == 1
        assert log.count(f"leave:immigrant:{i}") == 1
    for s in (51, 52):
        assert log.count(f"leave:spectator:{s}") == 1
