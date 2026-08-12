"""Koan 18 — Dining Savages (starter code). Edit this file only.

Guarantees: nobody takes a serving from an empty pot, the cook refills only
a pot that is truly empty, and the cook sleeps until a diner wakes him. The
pot object itself is NOT thread-safe — keeping every pot call exclusive is
part of your job.
"""

import threading


class Village:
    def __init__(self, m, pot):
        self.m = m      # servings added per refill
        self.pot = pot  # shared pot: put_servings(m) / get_serving()
        # TODO: the synchronization members you need.

    def start_cook(self):
        """Spawn the cook as a daemon thread.

        The cook loops forever: sleep until a diner reports the pot empty,
        refill it with pot.put_servings(self.m), then announce the refill.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("18_dining_savages: Village.start_cook")

    def dine(self):
        """Take exactly one serving via pot.get_serving().

        If the pot is empty, wake the cook and wait for the refill before
        taking your serving.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("18_dining_savages: Village.dine")
