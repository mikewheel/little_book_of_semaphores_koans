"""Koan 14 — No-starve mutex (starter code). Edit this file only.

Guarantee: acquire()/release() give mutual exclusion, and once a thread has
called acquire(), the number of times OTHER threads can be granted the lock
before it gets in is bounded — even though the only building block is a
semaphore that wakes waiters at random.

Honor rule: NoStarveMutex may use only WeakSemaphore instances and plain
integers. The tests read this class's source and reject
threading.Lock/Semaphore/Condition/Event inside it.
"""

import random
import threading


class WeakSemaphore:
    """A semaphore with only the *weak* guarantee (provided — do not modify).

    Two deliberately adversarial behaviors, both legal for a semaphore that
    promises no more than "a signal wakes someone":

    - release() wakes a RANDOM waiter — never assume first-come-first-served.
    - a release() that finds no waiters banks a token that any LATER arrival
      may snatch, even if an earlier thread was already mid-approach.
    """

    def __init__(self, value=0):
        self._lock = threading.Lock()
        self._value = value
        self._waiters = []

    def acquire(self):
        with self._lock:
            if self._value > 0:
                self._value -= 1
                return
            gate = threading.Event()
            self._waiters.append(gate)
        gate.wait()  # the token is handed to us directly by release()

    def release(self):
        with self._lock:
            if self._waiters:
                self._waiters.pop(random.randrange(len(self._waiters))).set()
            else:
                self._value += 1


class NoStarveMutex:
    """A mutex with bounded overtaking, built from weak semaphores alone."""

    def __init__(self):
        # TODO: your sync members — WeakSemaphore instances and plain ints
        # only (the honor-rule test reads this class's source).
        pass

    def acquire(self):
        """Block until the lock is yours. Overtaking must stay bounded."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("14_no_starve_mutex: NoStarveMutex.acquire")

    def release(self):
        """Hand the lock on. Pair with acquire()."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("14_no_starve_mutex: NoStarveMutex.release")
