"""Koan 04 — Multiplex (starter code). Edit this file only.

Guarantee: at most n threads are between enter() and exit() at any moment,
and n threads CAN be inside simultaneously.
"""

import threading


class Multiplex:
    def __init__(self, n):
        # TODO: create your semaphore, sized for the room.
        pass

    def enter(self):
        """Block until the room has capacity, then come in."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("04_multiplex: Multiplex.enter")

    def exit(self):
        """Leave the room, freeing a slot for one waiter (if any)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("04_multiplex: Multiplex.exit")
