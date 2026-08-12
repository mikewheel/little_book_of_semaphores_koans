"""Koan 03 — Mutex (starter code). Edit this file only.

Guarantee: between acquire() and release(), no other thread is between its
own acquire() and release(). Works for any number of threads.
"""

import threading


class Mutex:
    def __init__(self):
        # TODO: create your semaphore. The initial value is the whole game.
        pass

    def acquire(self):
        """Block until the critical section is free, then claim it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("03_mutex: Mutex.acquire")

    def release(self):
        """Leave the critical section, admitting one waiter (if any)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("03_mutex: Mutex.release")
