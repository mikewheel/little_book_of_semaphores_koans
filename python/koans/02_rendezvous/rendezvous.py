"""Koan 02 — Rendezvous (starter code). Edit this file only.

Guarantees: a1 happens before b2, and b1 happens before a2. The order of
a1 vs b1 must remain unconstrained.
"""

import threading


class Rendezvous:
    def __init__(self):
        # TODO: create the semaphores you need (initial values matter!).
        pass

    def run_a(self, a1, a2):
        """Thread A's body: a1(), meet B, then a2()."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("02_rendezvous: Rendezvous.run_a")

    def run_b(self, b1, b2):
        """Thread B's body: b1(), meet A, then b2()."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("02_rendezvous: Rendezvous.run_b")
