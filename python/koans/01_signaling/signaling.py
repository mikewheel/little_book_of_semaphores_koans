"""Koan 01 — Signaling (starter code). Edit this file only.

Guarantee: b1 never runs before a1 has completed, no matter how the
scheduler interleaves the two threads.
"""

import threading


class Signaling:
    def __init__(self):
        # TODO: create the semaphore(s) you need here.
        pass

    def run_a(self, a1):
        """Thread A's body: run a1(), then let thread B proceed.

        A must never block waiting for B.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("01_signaling: Signaling.run_a")

    def run_b(self, b1):
        """Thread B's body: run b1(), but only after A has finished a1().

        If B arrives first it must block (not spin) until A signals.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("01_signaling: Signaling.run_b")
