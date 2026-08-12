"""Koan 33 — Child care (starter code). Edit this file only.

Guarantee: at every moment, children inside <= ratio x adults inside.
child_enter blocks while admitting the child would break the bound;
adult_leave blocks while leaving would break it. adult_enter and
child_leave never block.
"""

import threading


class ChildCare:
    def __init__(self, ratio=3):
        self.ratio = ratio
        # TODO: your synchronization state here.

    def adult_enter(self):
        """An adult walks in. Never blocks."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("33_child_care: ChildCare.adult_enter")

    def adult_leave(self):
        """An adult walks out — but only once the invariant survives it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("33_child_care: ChildCare.adult_leave")

    def child_enter(self):
        """A child comes in, waiting at the door while the center is full."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("33_child_care: ChildCare.child_enter")

    def child_leave(self):
        """A child goes home. Never blocks."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("33_child_care: ChildCare.child_leave")
