"""Koan 25 — Roller coaster (starter code). Edit this file only.

Guarantee: per ride cycle the car loads, exactly `capacity` passengers
board, the car runs, the car unloads, and exactly those passengers
unboard — in that order. Passengers board only between load() and run(),
the car runs only when full, and the next load() waits until every rider
from the previous cycle has unboarded.
"""

import threading


class RollerCoaster:
    def __init__(self, capacity, hooks):
        # hooks.load()/run()/unload() are the car's phases;
        # hooks.board(pid)/unboard(pid) are the passenger's moves.
        self.capacity = capacity
        self.hooks = hooks
        # TODO: your synchronization members here.

    def passenger(self, pid):
        """One passenger takes one complete ride.

        Wait until boarding is allowed, call self.hooks.board(pid), ride,
        wait until unboarding is allowed, call self.hooks.unboard(pid),
        then return.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("25_roller_coaster: RollerCoaster.passenger")

    def start_car(self, n_rides):
        """The car's body — the tests run it on its own thread.

        Perform n_rides cycles: call self.hooks.load(), let exactly
        `capacity` passengers board, call self.hooks.run() only once the
        car is full, call self.hooks.unload(), and start the next cycle
        only after all `capacity` riders have unboarded. Return when all
        n_rides cycles are done.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("25_roller_coaster: RollerCoaster.start_car")
