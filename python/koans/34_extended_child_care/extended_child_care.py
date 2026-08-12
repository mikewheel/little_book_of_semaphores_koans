"""Koan 34 — Extended child care (starter code). Edit this file only.

Guarantees: at every moment, children inside <= ratio x adults inside
(an adult waiting to leave still counts as inside), and nobody waits
unnecessarily — a blocked adult_leave must not keep out a child the
ratio genuinely allows, and a waiting adult departs as soon as the
counts permit. adult_enter and child_leave never block.
"""

import threading


class ExtendedChildCare:
    def __init__(self, ratio=3):
        self.ratio = ratio
        # TODO: your synchronization state here.

    def adult_enter(self):
        """An adult walks in. Never blocks."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "34_extended_child_care: ExtendedChildCare.adult_enter"
        )

    def adult_leave(self):
        """An adult walks out the moment the invariant survives it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "34_extended_child_care: ExtendedChildCare.adult_leave"
        )

    def child_enter(self):
        """A child comes in, waiting only while the ratio truly forbids it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "34_extended_child_care: ExtendedChildCare.child_enter"
        )

    def child_leave(self):
        """A child goes home. Never blocks."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "34_extended_child_care: ExtendedChildCare.child_leave"
        )
