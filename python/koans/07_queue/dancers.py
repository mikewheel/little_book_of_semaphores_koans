"""Koan 07 — Queue (starter code). Edit this file only.

Guarantee: dancers proceed only in leader/follower pairs. An arriving
leader blocks until a follower is (or becomes) available, and vice versa.
"""

import threading


class DanceFloor:
    def __init__(self):
        # TODO: your semaphores here (initial values matter!).
        pass

    def leader_arrives(self):
        """Block until this leader has been matched with a follower."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("07_queue: DanceFloor.leader_arrives")

    def follower_arrives(self):
        """Block until this follower has been matched with a leader."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("07_queue: DanceFloor.follower_arrives")
