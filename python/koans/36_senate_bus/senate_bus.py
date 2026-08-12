"""Koan 36 — Senate bus (starter code). Edit this file only.

Guarantees: when a bus arrives, exactly the riders already waiting board
(never more than capacity); riders arriving mid-boarding wait for the
next bus; depart(n) reports exactly how many boarded this bus; a bus at
an empty stop departs immediately with depart(0).
"""

import threading


class BusStop:
    def __init__(self, capacity=50, board=None, depart=None):
        self.capacity = capacity
        # Test-supplied hooks (already wired — leave these two lines).
        self._board = board or (lambda rid: None)
        self._depart = depart or (lambda n: None)
        # TODO: your synchronization state here.

    def rider(self, rid):
        """Arrive at the stop, wait for a bus, board it. Returns once
        rider `rid` is aboard."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("36_senate_bus: BusStop.rider")

    def bus_arrives(self):
        """One bus visit: board the eligible waiting riders (up to
        capacity, none that arrived after the bus), depart(n), return."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("36_senate_bus: BusStop.bus_arrives")
