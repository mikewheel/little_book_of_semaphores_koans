"""Koan 20 — FIFO Barbershop (starter code). Edit this file only.

Guarantees: everything koan 19 promised — capacity n with balking, a
sleeping barber, 1:1 fully-finished haircuts — PLUS customers are served
in the order they arrived (arrival = the moment customer_visit registers
them, inside its mutual exclusion).
"""

import threading


class FifoBarbershop:
    def __init__(self, n):
        self.n = n  # max customers in the shop (waiting room + chair)
        # TODO: the synchronization members you need.

    def start_barber(self, cut_hair):
        """Spawn the barber as a daemon thread.

        Same as koan 19, with one addition: the barber must serve waiting
        customers strictly in their arrival order.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("20_fifo_barbershop: FifoBarbershop.start_barber")

    def customer_visit(self, get_hair_cut):
        """One customer's trip to the shop. Returns a bool.

        Balk with an immediate False if the shop holds n customers.
        Otherwise register your arrival, wait until the barber calls *you*
        (not just anyone), run get_hair_cut() concurrently with cut_hair(),
        and return True once the cut is fully done.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "20_fifo_barbershop: FifoBarbershop.customer_visit"
        )
