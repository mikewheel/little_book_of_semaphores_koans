"""Koan 15 — Dining philosophers (starter code). Edit this file only.

Guarantees: a fork is held by at most one philosopher at a time (so
neighbors never eat together), no deadlock even when everyone is hungry at
once, more than one philosopher CAN eat at the same time, and every
philosopher who keeps trying gets to eat.
"""

import threading


class Table:
    def __init__(self, n=5):
        self.n = n
        # TODO: per-fork exclusivity, plus whatever breaks the deadly cycle.

    def left(self, i):
        """Index of philosopher i's left fork (provided — free to use)."""
        return i

    def right(self, i):
        """Index of philosopher i's right fork (provided — free to use)."""
        return (i + 1) % self.n

    def get_forks(self, i):
        """Block until philosopher i holds BOTH adjacent forks."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("15_dining_philosophers: Table.get_forks")

    def put_forks(self, i):
        """Return philosopher i's two forks to the table."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("15_dining_philosophers: Table.put_forks")
