"""Koan 26 — Multi-car roller coaster (starter code). Edit this file only.

Guarantee: everything koan 25 promised, with several cars sharing one
track. Only one car boards passengers at a time; cars take the loading
dock in fixed rotation 0, 1, ..., n_cars-1, 0, ...; and because cars
cannot overtake on the track, they unload in the same order they loaded.
Boards of consecutive carloads never interleave.
"""

import threading


class MultiCarCoaster:
    def __init__(self, n_cars, capacity, hooks):
        # hooks.load(car)/run(car)/unload(car) are a car's phases (car is
        # the car id); hooks.board(pid)/unboard(pid) are a passenger's.
        self.n_cars = n_cars
        self.capacity = capacity
        self.hooks = hooks
        # TODO: your synchronization members here.

    def passenger(self, pid):
        """One passenger takes one complete ride in whichever car loads.

        Wait until boarding is allowed, call self.hooks.board(pid), ride,
        wait until unboarding is allowed, call self.hooks.unboard(pid),
        then return.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "26_multi_car_roller_coaster: MultiCarCoaster.passenger"
        )

    def start_cars(self, n_rides_per_car):
        """Run all n_cars cars concurrently; return when all have finished.

        Give each car its own thread. Car i performs n_rides_per_car
        cycles of load/run/unload under the constraints above (loading in
        rotation starting with car 0, unloading in loading order).
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "26_multi_car_roller_coaster: MultiCarCoaster.start_cars"
        )
