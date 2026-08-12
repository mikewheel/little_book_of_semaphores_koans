"""Koan 09 — Producer-consumer (starter code). Edit this file only.

Guarantees: at most one thread touches the (not thread-safe) buffer at a
time; consume() blocks while the buffer is empty; produce() never blocks
indefinitely. The buffer is unbounded in this koan.
"""

import threading


class ProducerConsumer:
    def __init__(self, buffer):
        # `buffer` has add(item) and get() -> item. It is NOT thread-safe,
        # and get() on an empty buffer is an error.
        self.buffer = buffer
        # TODO: create your sync members — something granting exclusive
        # access to the buffer, and something counting what's in it.

    def produce(self, item):
        """Put `item` into the buffer. Must never block indefinitely."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("09_producer_consumer: ProducerConsumer.produce")

    def consume(self):
        """Block while the buffer is empty, then remove and return an item."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("09_producer_consumer: ProducerConsumer.consume")
