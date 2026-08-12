from koan_utils import EventLog, ThreadRunner, jitter

from multicar_coaster import MultiCarCoaster

N_CARS = 3
CAPACITY = 2


class CoasterHooks:
    """Records every phase; dawdles inside each hook to widen windows."""

    def __init__(self):
        self.log = EventLog()

    def load(self, car):
        jitter(2.0)
        self.log.record(f"load:{car}")

    def run(self, car):
        jitter(2.0)
        self.log.record(f"run:{car}")

    def unload(self, car):
        jitter(2.0)
        self.log.record(f"unload:{car}")

    def board(self, pid):
        jitter(2.0)
        self.log.record(f"board:{pid}")

    def unboard(self, pid):
        jitter(2.0)
        self.log.record(f"unboard:{pid}")

    def events(self):
        return self.log.events()

    def cars_of(self, prefix):
        return [
            int(e.split(":")[1]) for e in self.events() if e.startswith(prefix)
        ]


def expected_rotation(rides_per_car):
    return list(range(N_CARS)) * rides_per_car


def assert_one_car_boarding_at_a_time(events, capacity):
    """Each load opens a boarding window; exactly `capacity` boards must
    land inside it before any other car may call load."""
    open_car = None
    boards_in_window = 0
    for e in events:
        if e.startswith("load:"):
            car = int(e.split(":")[1])
            assert open_car is None, (
                f"car {car} called load while car {open_car} was still "
                f"boarding: {events}"
            )
            open_car = car
            boards_in_window = 0
        elif e.startswith("board:"):
            assert open_car is not None, (
                f"a passenger boarded outside any boarding window: {events}"
            )
            boards_in_window += 1
            if boards_in_window == capacity:
                open_car = None  # window complete
    assert open_car is None, (
        f"car {open_car}'s boarding window never filled: {events}"
    )


def assert_unloads_are_whole(events, capacity):
    """Each unload is followed by exactly `capacity` unboards before the
    next unload."""
    open_car = None
    unboards_in_window = 0
    for e in events:
        if e.startswith("unload:"):
            car = int(e.split(":")[1])
            assert open_car is None, (
                f"car {car} called unload while car {open_car}'s riders "
                f"were still getting off: {events}"
            )
            open_car = car
            unboards_in_window = 0
        elif e.startswith("unboard:"):
            assert open_car is not None, (
                f"a passenger unboarded outside any unloading window: {events}"
            )
            unboards_in_window += 1
            if unboards_in_window == capacity:
                open_car = None
    assert open_car is None, (
        f"car {open_car}'s unloading window never emptied: {events}"
    )


def run_park(rides_per_car, join_timeout=15):
    n_passengers = N_CARS * CAPACITY * rides_per_car
    hooks = CoasterHooks()
    coaster = MultiCarCoaster(N_CARS, CAPACITY, hooks)
    runner = ThreadRunner()
    runner.spawn(coaster.start_cars, rides_per_car, name="cars")
    for pid in range(n_passengers):
        def body(pid=pid):
            jitter()
            coaster.passenger(pid)

        runner.spawn(body, name=f"passenger-{pid}")
    runner.join_all(timeout=join_timeout)
    return hooks


def test_loads_rotate_in_order():
    hooks = run_park(2)
    assert hooks.cars_of("load:") == expected_rotation(2), (
        f"cars must load in rotation 0,1,2,0,1,2 — saw {hooks.cars_of('load:')}"
    )


def test_unloads_match_load_order():
    hooks = run_park(2)
    loads = hooks.cars_of("load:")
    unloads = hooks.cars_of("unload:")
    assert unloads == loads, (
        f"cars cannot pass each other: unload order {unloads} must equal "
        f"load order {loads}"
    )


def test_one_car_boarding_at_a_time():
    hooks = run_park(2)
    assert_one_car_boarding_at_a_time(hooks.events(), CAPACITY)


def test_all_passengers_ride():
    hooks = run_park(2)
    events = hooks.events()
    boarded = sorted(
        int(e.split(":")[1]) for e in events if e.startswith("board:")
    )
    unboarded = sorted(
        int(e.split(":")[1]) for e in events if e.startswith("unboard:")
    )
    assert boarded == list(range(12)), (
        f"every passenger must board exactly once, saw {boarded}"
    )
    assert unboarded == list(range(12))
    assert len(hooks.cars_of("run:")) == 6


def test_stress():
    hooks = run_park(3, join_timeout=20)
    events = hooks.events()
    assert hooks.cars_of("load:") == expected_rotation(3)
    assert hooks.cars_of("unload:") == expected_rotation(3)
    assert_one_car_boarding_at_a_time(events, CAPACITY)
    assert_unloads_are_whole(events, CAPACITY)
    boarded = sorted(
        int(e.split(":")[1]) for e in events if e.startswith("board:")
    )
    assert boarded == list(range(18))
