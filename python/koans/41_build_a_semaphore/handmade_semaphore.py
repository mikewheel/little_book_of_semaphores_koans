"""Koan 41 — Build a semaphore (starter code). Edit this file only.

Guarantees being built (the semaphore properties, from the book):
1. acquire() blocks while the value is exhausted and proceeds otherwise;
2. release() banks a permit that a future acquire() can spend — signals
   are never lost, even with nobody waiting yet;
3. when a release() wakes the waiters, one of the threads that was
   actually waiting gets in — a fresh caller racing in cannot snatch
   that wakeup.

House rule (the whole point of the koan): build it from a lock and a
condition variable only. The stdlib's ready-made semaphore classes are
off-limits, and the tests read this file's source to keep you honest.
"""

import threading


class HandmadeSemaphore:
    def __init__(self, value=0):
        if value < 0:
            raise ValueError("initial semaphore value must be nonnegative")
        self.value = value
        # TODO: your lock / condition machinery (and one more counter —
        # see the hints if the third guarantee gets slippery).

    def acquire(self):
        """Take a permit, blocking until one is available."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("41_build_a_semaphore: HandmadeSemaphore.acquire")

    def release(self):
        """Bank one permit and, if anyone is waiting, wake exactly one."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("41_build_a_semaphore: HandmadeSemaphore.release")
