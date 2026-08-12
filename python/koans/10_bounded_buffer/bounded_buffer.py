"""Koan 10 — Bounded buffer (starter code). Edit this file only.

Guarantees: at most one thread touches the (not thread-safe) buffer at a
time; consume() blocks while the buffer is empty; produce() blocks while
the buffer already holds `capacity` items, so it never overfills.
"""

import threading


class BoundedBuffer:
    def __init__(self, buffer, capacity):
        # `buffer` has add(item) and get() -> item. It is NOT thread-safe,
        # and get() on an empty buffer is an error.
        self.buffer = buffer
        self.capacity = capacity
        # TODO: create your sync members. Koan 09's roster, plus one more
        # to account for room that hasn't been used yet.

    def produce(self, item):
        """Block while the buffer is full, then put `item` into it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("10_bounded_buffer: BoundedBuffer.produce")

    def consume(self):
        """Block while the buffer is empty, then remove and return an item."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("10_bounded_buffer: BoundedBuffer.consume")
