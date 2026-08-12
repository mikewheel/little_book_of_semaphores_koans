"""Koan 06 — Reusable barrier (starter code). Edit this file only.

Guarantee: threads call wait() in a loop. In every round, no thread
returns from wait() until all n threads have entered it that round, and
no thread can start the next round's wait() while a straggler is still
leaving this one (no lapping).
"""

import threading


class ReusableBarrier:
    def __init__(self, n):
        self.n = n
        # TODO: a counter, something to protect it, and whatever the two
        # phases need to sleep on.

    def phase1(self):
        """Arrival phase: block until all n threads have called this."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("06_reusable_barrier: ReusableBarrier.phase1")

    def phase2(self):
        """Departure phase: block until all n threads are clear to loop."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("06_reusable_barrier: ReusableBarrier.phase2")

    def wait(self):
        """Arrive at the barrier; return when the whole cohort may proceed."""
        self.phase1()
        self.phase2()
