"""Koan 40 — Extended Dining Hall (starter code). Edit this file only.

Guarantees being built: nobody eats alone and nobody is left eating alone.
A student may not START dining while the table is empty and no other
student is ready to eat (she waits for company; the pair sits down
together). And, as in koan 39, a finished student may not leave if that
would strand exactly one diner with no other leaver for company.
"""

import threading


class ExtendedDiningHall:
    def __init__(self, hooks):
        self.hooks = hooks
        # TODO: your synchronization members.

    def student(self, sid, dine_gate=None):
        """One student's meal: get food, dine, leave.

        Calls hooks.get_food(sid) first — after it returns she is "ready
        to eat". hooks.dine(sid) may only fire when the sitting-down rule
        allows. If ``dine_gate`` is not None, call it after the dine hook
        returns — it blocks until the tests decide she is done eating.
        Then she is "ready to leave", and hooks.leave(sid) may only fire
        when the leaving rule allows.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "40_extended_dining_hall: ExtendedDiningHall.student"
        )
