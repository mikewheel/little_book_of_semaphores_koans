"""Koan 30 — Baboon crossing (starter code). Edit this file only.

Guarantees: eastbound and westbound baboons are never on the rope at the
same time; at most `capacity` baboons on the rope; same-direction baboons
share the rope up to capacity; and no direction can starve the other —
later opposing arrivals cannot overtake a baboon that is already waiting.
"""

import threading


class Rope:
    def __init__(self, capacity=5):
        self.capacity = capacity
        # TODO: create your sync members here.

    def east_enter(self):
        """Block until this eastbound baboon may get on the rope."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("30_baboon_crossing: Rope.east_enter")

    def east_exit(self):
        """Step off the rope (caller entered via east_enter)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("30_baboon_crossing: Rope.east_exit")

    def west_enter(self):
        """Block until this westbound baboon may get on the rope."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("30_baboon_crossing: Rope.west_enter")

    def west_exit(self):
        """Step off the rope (caller entered via west_enter)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("30_baboon_crossing: Rope.west_exit")
