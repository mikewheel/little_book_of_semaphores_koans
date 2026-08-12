"""Koan 24 — River crossing (starter code). Edit this file only.

Guarantee: threads cross the river only in legal boatloads of exactly
four — four hackers, four serfs, or two of each. All four board() calls of
a boatload happen before any board() of the next boatload, and exactly one
passenger per boatload calls row_boat() after everyone has boarded.
"""

import threading


class Boat:
    def __init__(self, hooks):
        # hooks.board(kind) as a passenger boards; hooks.row_boat(kind) by
        # the one rower per boatload. kind is "hacker" or "serf".
        self.hooks = hooks
        # TODO: your synchronization members here.

    def hacker_arrives(self):
        """One hacker reaches the dock.

        Block until this thread belongs to a legal boatload of four, call
        self.hooks.board("hacker"), and — if this thread ends up rowing —
        call self.hooks.row_boat("hacker") once all four have boarded.
        Return only when this boatload has sailed.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("24_river_crossing: Boat.hacker_arrives")

    def serf_arrives(self):
        """One serf reaches the dock. Same contract, kind "serf"."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("24_river_crossing: Boat.serf_arrives")
