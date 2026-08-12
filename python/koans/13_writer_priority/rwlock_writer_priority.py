"""Koan 13 — Writer-priority readers-writers (starter code). Edit this file only.

Guarantees: readers share; a writer is alone; and once any writer is
waiting or writing, no NEW reader enters until every queued writer has
finished. (Readers can starve under a steady writer stream — by design.)
"""

import threading


class WriterPriorityReadWriteLock:
    def __init__(self):
        # TODO: your sync members. Koan 11's collective-claim trick is
        # needed twice here — once per category.
        pass

    def reader_enter(self):
        """Block while any writer is inside OR queued; readers share."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "13_writer_priority: WriterPriorityReadWriteLock.reader_enter"
        )

    def reader_exit(self):
        """Leave the room. The last reader out has a special job."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "13_writer_priority: WriterPriorityReadWriteLock.reader_exit"
        )

    def writer_enter(self):
        """Bar new readers the moment you queue; enter once alone."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "13_writer_priority: WriterPriorityReadWriteLock.writer_enter"
        )

    def writer_exit(self):
        """Hand off to the next writer if any; else readmit readers."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "13_writer_priority: WriterPriorityReadWriteLock.writer_exit"
        )
