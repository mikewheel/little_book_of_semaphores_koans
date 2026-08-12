"""Koan 12 — No-starve readers-writers (starter code). Edit this file only.

Guarantees: koan 11's safety rules (readers share; a writer is alone),
plus fairness for writers: readers arriving AFTER a waiting writer do not
enter before it. Incumbent readers finish; the writer goes next.
"""

import threading


class NoStarveReadWriteLock:
    def __init__(self):
        # TODO: koan 11's members, plus a doorway that a waiting writer
        # can hold shut.
        pass

    def reader_enter(self):
        """Block while a writer is inside OR waiting; readers still share."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "12_no_starve_readers_writers: NoStarveReadWriteLock.reader_enter"
        )

    def reader_exit(self):
        """Leave the room. The last reader out has a special job."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "12_no_starve_readers_writers: NoStarveReadWriteLock.reader_exit"
        )

    def writer_enter(self):
        """Queue up, bar later arrivals, wait for the room to empty."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "12_no_starve_readers_writers: NoStarveReadWriteLock.writer_enter"
        )

    def writer_exit(self):
        """Give up the room and reopen the doorway."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "12_no_starve_readers_writers: NoStarveReadWriteLock.writer_exit"
        )
