"""Koan 29 — No-starve unisex bathroom (starter code). Edit this file only.

Guarantees: men and women are never inside together; at most `capacity`
people inside; same gender shares the room; and nobody starves — opposite-
gender arrivals that show up after someone is already waiting cannot get
in ahead of them.
"""

import threading


class FairBathroom:
    def __init__(self, capacity=3):
        self.capacity = capacity
        # TODO: create your sync members here.

    def male_enter(self):
        """Block until this man may enter without breaking the rules."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("29_no_starve_bathroom: FairBathroom.male_enter")

    def male_exit(self):
        """Leave the bathroom (caller entered via male_enter)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("29_no_starve_bathroom: FairBathroom.male_exit")

    def female_enter(self):
        """Block until this woman may enter without breaking the rules."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("29_no_starve_bathroom: FairBathroom.female_enter")

    def female_exit(self):
        """Leave the bathroom (caller entered via female_enter)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("29_no_starve_bathroom: FairBathroom.female_exit")
