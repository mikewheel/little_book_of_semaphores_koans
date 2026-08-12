"""Koan 35 — Room party (starter code). Edit this file only.

Guarantees: the dean enters only an empty room (search) or an
over-threshold party (breakup), waiting outside otherwise; while the dean
is inside no student enters but students may leave; after a breakup the
dean stays until the room is empty. One dean, any number of students.
"""

import threading


class Room:
    def __init__(self, threshold=50, search=None, breakup=None, party=None):
        self.threshold = threshold
        # Test-supplied hooks (already wired — leave these three lines).
        self._search = search or (lambda: None)
        self._breakup = breakup or (lambda: None)
        self._party = party or (lambda sid: None)
        # TODO: your synchronization state here.

    def student_visit(self, sid):
        """Enter (waiting out the dean), party(sid), then leave."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("35_room_party: Room.student_visit")

    def dean_visit(self):
        """Search an empty room, or break up a big party and hold the door
        until it empties; with 1..threshold students inside, wait for one
        of those conditions. Returns when the dean leaves."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("35_room_party: Room.dean_visit")
