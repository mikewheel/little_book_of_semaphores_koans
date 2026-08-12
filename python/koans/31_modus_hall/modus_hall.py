"""Koan 31 — The Modus Hall problem (starter code). Edit this file only.

Guarantees: heathens and prudes are never on the path together; a faction
shares the path freely with itself; an empty path goes to the first
arrival; and control flips by majority rule — when the queued opposition
outnumbers the current holders, new holders are barred, incumbents finish,
and the whole waiting cohort crosses. A minority keeps waiting.
"""

import threading


class Path:
    def __init__(self):
        # TODO: create your sync members here.
        pass

    def heathen_cross(self, cross):
        """Arrive as a heathen; wait if required; run cross() on the path."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("31_modus_hall: Path.heathen_cross")

    def prude_cross(self, cross):
        """Arrive as a prude; wait if required; run cross() on the path."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("31_modus_hall: Path.prude_cross")
