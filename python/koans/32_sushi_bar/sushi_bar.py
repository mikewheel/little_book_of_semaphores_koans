"""Koan 32 — The sushi bar problem (starter code). Edit this file only.

Guarantees: at most `seats` customers eat at once; arrivals seat
immediately while a seat is free and the bar has not filled; once the bar
fills, later arrivals wait until it is COMPLETELY empty, then the waiting
cohort (up to `seats`) is seated together.
"""

import threading


class SushiBar:
    def __init__(self, seats=5):
        self.seats = seats
        # TODO: create your scoreboard and sync members here.

    def dine(self, eat):
        """Arrive; wait if the rules demand it; run eat() while seated."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("32_sushi_bar: SushiBar.dine")
