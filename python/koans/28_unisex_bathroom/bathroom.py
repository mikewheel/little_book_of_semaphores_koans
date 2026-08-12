"""Koan 28 — Unisex bathroom (starter code). Edit this file only.

Guarantee: the two genders are never inside at the same time; at most
`capacity` people are inside at once; and up to `capacity` people of the
same gender can share. (Starvation is allowed in this koan — a stream of
one gender may shut the other out indefinitely. Koan 29 fixes that.)
"""

import threading


class Bathroom:
    def __init__(self, capacity=3):
        self.capacity = capacity
        # TODO: your synchronization members here.

    def female_enter(self):
        """Block until entering is allowed: no men inside, and fewer than
        `capacity` women inside."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("28_unisex_bathroom: Bathroom.female_enter")

    def female_exit(self):
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("28_unisex_bathroom: Bathroom.female_exit")

    def male_enter(self):
        """Block until entering is allowed: no women inside, and fewer
        than `capacity` men inside."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("28_unisex_bathroom: Bathroom.male_enter")

    def male_exit(self):
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("28_unisex_bathroom: Bathroom.male_exit")
