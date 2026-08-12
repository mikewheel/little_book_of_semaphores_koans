"""Koan 11 — Readers-writers (starter code). Edit this file only.

Guarantees: any number of readers may hold the lock together; a writer
holds it alone — no readers, no other writers. (Writer starvation is NOT
addressed here; that's koan 12.)
"""

import threading


class ReadWriteLock:
    def __init__(self):
        # TODO: create your sync members — something writers hold
        # exclusively, plus whatever the readers need to hold it
        # collectively.
        pass

    def reader_enter(self):
        """Block while a writer is inside; readers never block readers."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("11_readers_writers: ReadWriteLock.reader_enter")

    def reader_exit(self):
        """Leave the room. The last reader out has a special job."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("11_readers_writers: ReadWriteLock.reader_exit")

    def writer_enter(self):
        """Block until the room is completely empty, then own it."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("11_readers_writers: ReadWriteLock.writer_enter")

    def writer_exit(self):
        """Give up ownership of the room."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("11_readers_writers: ReadWriteLock.writer_exit")
