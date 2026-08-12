"""Koan 08 — Exclusive queue (starter code). Edit this file only.

Guarantees: dancers pair up leader/follower; at most one pair is on the
floor at a time (the pair's two dance callbacks overlap; no other dance
overlaps them); a leader does not return until its partner's dance has
completed.
"""

import threading


class ExclusiveDanceFloor:
    def __init__(self):
        # TODO: waiting-dancer counters, something to protect them, and
        # queues to park on.
        pass

    def leader_dances(self, dance):
        """Pair with a follower, run dance() with the floor to yourselves,
        and return only after the partner's dance has completed."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "08_exclusive_queue: ExclusiveDanceFloor.leader_dances"
        )

    def follower_dances(self, dance):
        """Pair with a leader, then run dance() with the floor to yourselves."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "08_exclusive_queue: ExclusiveDanceFloor.follower_dances"
        )
