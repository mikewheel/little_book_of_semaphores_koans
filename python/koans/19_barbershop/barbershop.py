"""Koan 19 — Barbershop (starter code). Edit this file only.

Guarantees: at most n customers in the shop (arrivals beyond that balk and
leave with False); the barber sleeps until a customer is present; each
cut_hair() is paired with exactly one customer's get_hair_cut(), and a
customer's visit only succeeds once their cut is fully finished.
"""

import threading


class Barbershop:
    def __init__(self, n):
        self.n = n  # max customers in the shop (waiting room + chair)
        # TODO: the synchronization members you need.

    def start_barber(self, cut_hair):
        """Spawn the barber as a daemon thread.

        The barber loops forever: sleep until a customer is present, then
        call cut_hair() exactly once for that customer, and don't move on
        to the next customer until this one's haircut is fully done.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("19_barbershop: Barbershop.start_barber")

    def customer_visit(self, get_hair_cut):
        """One customer's trip to the shop. Returns a bool.

        If the shop already holds n customers, leave immediately and return
        False (a "balk") — without blocking. Otherwise wait your turn, call
        get_hair_cut() while the barber runs cut_hair(), and return True
        only after both sides of the haircut have finished.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("19_barbershop: Barbershop.customer_visit")
