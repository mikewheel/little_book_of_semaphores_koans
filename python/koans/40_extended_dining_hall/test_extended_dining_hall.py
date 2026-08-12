import random
import threading
import time
import types

from koan_utils import EventLog, ThreadRunner, jitter

from extended_dining_hall import ExtendedDiningHall


def make_hall(log, gates=None):
    """Build a hall whose hooks record into ``log``.

    ``gates`` maps an event label to a threading.Event; a gated hook records
    ``<label>:pending`` and then blocks until its gate is set, only recording
    the bare label once released. A gated ``dine:<sid>`` therefore means
    "student <sid> is at the table eating" until the test sets her gate.
    """
    gates = gates or {}

    def fire(label):
        gate = gates.get(label)
        if gate is not None:
            log.record(label + ":pending")
            gate.wait(20)
        log.record(label)

    hooks = types.SimpleNamespace(
        get_food=lambda sid: fire(f"food:{sid}"),
        dine=lambda sid: fire(f"dine:{sid}"),
        leave=lambda sid: fire(f"leave:{sid}"),
    )
    return ExtendedDiningHall(hooks)


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


def test_first_student_waits_to_dine():
    log = EventLog()
    hall = make_hall(log)
    runner = ThreadRunner()
    runner.spawn(hall.student, 1, name="s1")
    wait_logged(runner, log, "food:1")  # she has her tray...
    time.sleep(0.3)
    runner.raise_worker_errors()
    assert log.count("dine:1") == 0, (
        f"student 1 sat down to eat all alone; log was {log.events()}"
    )
    runner.spawn(hall.student, 2, name="s2")  # company arrives
    wait_logged(runner, log, "dine:1")  # they sit down together
    wait_logged(runner, log, "dine:2")
    runner.join_all(timeout=10)
    assert log.count("leave:1") == 1
    assert log.count("leave:2") == 1


def test_joins_existing_diner_immediately():
    log = EventLog()
    gates = {f"dine:{i}": threading.Event() for i in (1, 2, 3)}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall.student, 1, name="s1")
        runner.spawn(hall.student, 2, name="s2")
        wait_logged(runner, log, "dine:1:pending")  # the pair is eating
        wait_logged(runner, log, "dine:2:pending")
        runner.spawn(hall.student, 3, name="s3")
        # Someone is already dining, so student 3 sits down promptly.
        wait_logged(runner, log, "dine:3:pending")
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    for i in (1, 2, 3):
        assert log.count(f"leave:{i}") == 1


def test_early_finisher_waits():
    log = EventLog()
    gates = {"dine:1": threading.Event(), "dine:2": threading.Event()}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall.student, 1, name="s1")
        runner.spawn(hall.student, 2, name="s2")
        wait_logged(runner, log, "dine:1:pending")  # both at the table
        wait_logged(runner, log, "dine:2:pending")
        gates["dine:1"].set()  # student 1 finishes first
        time.sleep(0.3)
        runner.raise_worker_errors()
        assert log.count("leave:1") == 0, (
            "student 1 walked out and stranded student 2 eating alone; "
            f"log was {log.events()}"
        )
        gates["dine:2"].set()  # student 2 finishes: they go together
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    assert log.count("leave:1") == 1
    assert log.count("leave:2") == 1


def test_newcomer_releases_waiter():
    log = EventLog()
    gates = {f"dine:{i}": threading.Event() for i in (1, 2, 3)}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        runner.spawn(hall.student, 1, name="s1")
        runner.spawn(hall.student, 2, name="s2")
        wait_logged(runner, log, "dine:1:pending")
        wait_logged(runner, log, "dine:2:pending")
        gates["dine:1"].set()  # student 1 is done, student 2 eats on
        time.sleep(0.1)  # give 1 a moment to get stuck politely
        runner.spawn(hall.student, 3, name="s3")  # newcomer joins the table
        wait_logged(runner, log, "dine:3:pending")
        # With 2 and 3 at the table, student 1 is free to go.
        wait_logged(runner, log, "leave:1")
        assert log.count("leave:2") == 0
        assert log.count("leave:3") == 0
        gates["dine:2"].set()  # now 2 finishes; 3 would be stranded
        time.sleep(0.3)
        runner.raise_worker_errors()
        assert log.count("leave:2") == 0, (
            "student 2 walked out and stranded student 3 eating alone; "
            f"log was {log.events()}"
        )
        gates["dine:3"].set()  # 3 finishes: 2 and 3 go together
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    for i in (1, 2, 3):
        assert log.count(f"leave:{i}") == 1


def test_pairs_leave_together():
    log = EventLog()
    hall = make_hall(log)
    runner = ThreadRunner()
    gate1, gate2 = threading.Event(), threading.Event()
    try:
        runner.spawn(hall.student, 1, gate1.wait, name="s1")
        runner.spawn(hall.student, 2, gate2.wait, name="s2")
        wait_logged(runner, log, "dine:1")  # seated together
        wait_logged(runner, log, "dine:2")
        gate1.set()  # both finish (near-)simultaneously
        gate2.set()
    finally:
        gate1.set()
        gate2.set()
    runner.join_all(timeout=10)
    assert log.count("leave:1") == 1
    assert log.count("leave:2") == 1


def test_full_lifecycle_stress():
    n = 12
    log = EventLog()
    gates = {f"dine:{i}": threading.Event() for i in range(n)}
    hall = make_hall(log, gates)
    runner = ThreadRunner()
    try:
        for i in range(n):  # staggered arrivals
            runner.spawn(hall.student, i, name=f"s{i}")
            jitter(8)
        for i in range(n):  # everyone ends up seated
            wait_logged(runner, log, f"dine:{i}:pending")
        order = list(range(n))
        random.shuffle(order)
        for i in order:  # they finish eating in random order
            gates[f"dine:{i}"].set()
            jitter(8)
    finally:
        release_all(gates)
    runner.join_all(timeout=10)
    for i in range(n):
        assert log.count(f"food:{i}") == 1
        assert log.count(f"dine:{i}") == 1
        assert log.count(f"leave:{i}") == 1
