"""Koan 21 — Hilzer's Barbershop (starter code). Edit this file only.

Guarantees: at most `capacity` customers in the shop (late arrivals balk
with False); at most `sofa_size` customers on the sofa; customers go from
sofa to barber chair in sofa-seating order; at most `n_barbers` haircuts at
once (and that many really can happen at once); each customer pays and has
the payment accepted — at one cash register, one at a time — before their
visit completes.

Hook contract (each takes the customer id `cid` or barber id `bid`):
- every served customer's thread calls, in order:
  enter_shop(cid) → sit_on_sofa(cid) → sit_in_chair(cid) → pay(cid)
- a customer occupies the shop from enter_shop(cid) until after payment is
  accepted; a sofa seat from sit_on_sofa(cid) until sit_in_chair(cid) has
  returned (only then may the next customer take the seat).
- each barber thread calls cut_hair(bid) once per customer served and
  accept_payment(bid) once per payment taken.
"""

import threading


class HilzersBarbershop:
    def __init__(self, capacity, sofa_size, n_barbers, hooks):
        self.capacity = capacity
        self.sofa_size = sofa_size
        self.n_barbers = n_barbers
        self.hooks = hooks
        # TODO: the synchronization members you need.

    def start_barbers(self):
        """Spawn n_barbers barber daemons (bid = 0 .. n_barbers-1).

        Each barber loops forever: sleep until a customer is ready for a
        chair, call that customer (oldest sofa-sitter first!), cut_hair(bid)
        for exactly that customer, then take a payment: accept_payment(bid)
        with the cash register to yourself.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "21_hilzers_barbershop: HilzersBarbershop.start_barbers"
        )

    def customer_visit(self, cid):
        """One customer's trip through the shop. Returns a bool.

        Balk with an immediate False if `capacity` customers are already
        inside. Otherwise walk the whole pipeline — enter_shop, sit_on_sofa
        (waiting for a free seat), sit_in_chair (waiting for a barber to
        call you, in sofa order), pay — and return True once your payment
        has been accepted.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "21_hilzers_barbershop: HilzersBarbershop.customer_visit"
        )
