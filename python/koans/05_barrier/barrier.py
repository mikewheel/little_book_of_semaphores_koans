"""Koan 05 — Barrier (starter code). Edit this file only.

Guarantee: no call to wait() returns until all n threads have called it.
One-shot: the barrier only needs to work for a single batch.
"""

import threading


class Barrier:
    def __init__(self, n):
        self.n = n
        # TODO: a counter, something to protect it, and something for
        # early arrivals to sleep on.

    def wait(self):
        """Block until n threads (including this one) have called wait()."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("05_barrier: Barrier.wait")
