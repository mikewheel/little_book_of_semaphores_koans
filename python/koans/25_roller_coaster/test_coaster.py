import time

from koan_utils import EventLog, ThreadRunner, jitter

from coaster import RollerCoaster

CAPACITY = 4


class CoasterHooks:
    """Records every phase; dawdles inside each hook to widen windows."""

    def __init__(self):
        self.log = EventLog()

    def load(self):
        jitter(2.0)
        self.log.record("load")

    def run(self):
        jitter(2.0)
        self.log.record("run")

    def unload(self):
        jitter(2.0)
        self.log.record("unload")

    def board(self, pid):
        jitter(2.0)
        self.log.record(f"board:{pid}")

    def unboard(self, pid):
        jitter(2.0)
        self.log.record(f"unboard:{pid}")

    def events(self):
        return self.log.events()


def assert_cycle_pattern(events, capacity, rides):
    """The exact sequence: (load, C boards, run, unload, C unboards) × rides."""
    i = 0
    for r in range(rides):
        assert i < len(events) and events[i] == "load", (
            f"cycle {r}: expected 'load' at position {i}: {events}"
        )
        i += 1
        for _ in range(capacity):
            assert i < len(events) and events[i].startswith("board:"), (
                f"cycle {r}: expected a board at position {i}: {events}"
            )
            i += 1
        assert i < len(events) and events[i] == "run", (
            f"cycle {r}: expected 'run' after {capacity} boards at {i}: {events}"
        )
        i += 1
        assert i < len(events) and events[i] == "unload", (
            f"cycle {r}: expected 'unload' at position {i}: {events}"
        )
        i += 1
        for _ in range(capacity):
            assert i < len(events) and events[i].startswith("unboard:"), (
                f"cycle {r}: expected an unboard at position {i}: {events}"
            )
            i += 1
    assert i == len(events), f"unexpected extra events after ride {rides}: {events}"


def run_park(n_passengers, n_rides, join_timeout=15):
    hooks = CoasterHooks()
    rc = RollerCoaster(CAPACITY, hooks)
    runner = ThreadRunner()
    runner.spawn(rc.start_car, n_rides, name="car")
    for pid in range(n_passengers):
        def body(pid=pid):
            jitter()
            rc.passenger(pid)

        runner.spawn(body, name=f"passenger-{pid}")
    runner.join_all(timeout=join_timeout)
    return hooks


def test_cycle_ordering():
    hooks = run_park(4, 1, join_timeout=10)
    events = hooks.events()
    assert_cycle_pattern(events, CAPACITY, 1)
    assert len([e for e in events if e.startswith("board:")]) == 4
    assert len([e for e in events if e.startswith("unboard:")]) == 4


def test_car_waits_for_full_load():
    hooks = CoasterHooks()
    rc = RollerCoaster(CAPACITY, hooks)
    runner = ThreadRunner()
    runner.spawn(rc.start_car, 1, name="car")
    for pid in range(3):
        runner.spawn(rc.passenger, pid, name=f"passenger-{pid}")
    time.sleep(0.4)  # 3 of 4 seats filled: give an early run() every chance
    runner.raise_worker_errors()
    assert "run" not in hooks.events(), (
        f"the car ran with only 3 of {CAPACITY} passengers: {hooks.events()}"
    )
    runner.spawn(rc.passenger, 3, name="passenger-3")  # the last seat
    runner.join_all(timeout=10)
    assert_cycle_pattern(hooks.events(), CAPACITY, 1)


def test_passengers_wait_for_car():
    hooks = CoasterHooks()
    rc = RollerCoaster(CAPACITY, hooks)
    runner = ThreadRunner()
    for pid in range(4):
        runner.spawn(rc.passenger, pid, name=f"passenger-{pid}")
    time.sleep(0.3)  # no car yet: nobody may board
    runner.raise_worker_errors()
    assert hooks.events() == [], (
        f"passengers boarded before the car called load(): {hooks.events()}"
    )
    runner.spawn(rc.start_car, 1, name="car")
    runner.join_all(timeout=10)
    assert_cycle_pattern(hooks.events(), CAPACITY, 1)


def test_multiple_rides():
    hooks = run_park(8, 2)
    events = hooks.events()
    assert_cycle_pattern(events, CAPACITY, 2)
    riders = sorted(int(e.split(":")[1]) for e in events if e.startswith("board:"))
    assert riders == list(range(8)), (
        f"every passenger must ride exactly once, saw pids {riders}"
    )
    unriders = sorted(int(e.split(":")[1]) for e in events if e.startswith("unboard:"))
    assert unriders == list(range(8))


def test_stress():
    hooks = run_park(12, 3, join_timeout=20)
    events = hooks.events()
    assert_cycle_pattern(events, CAPACITY, 3)
    riders = sorted(int(e.split(":")[1]) for e in events if e.startswith("board:"))
    assert riders == list(range(12))
