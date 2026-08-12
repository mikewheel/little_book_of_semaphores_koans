"""Koan 39 — Dining Hall (starter code). Edit this file only.

Guarantee being built: no student is ever stranded eating alone. A student
who has finished dining may only fire her leave hook if doing so would not
leave exactly one other student still eating with nobody else about to go;
the stranded situation resolves when a newcomer starts dining or the lone
diner finishes (in which case the two leave together).
"""

import threading


class DiningHall:
    def __init__(self, hooks):
        self.hooks = hooks
        # TODO: your synchronization members.

    def student(self, sid, dine_gate=None):
        """One student's meal, start to finish.

        Calls hooks.dine(sid) when she sits down to eat. If ``dine_gate``
        is not None, call it after the dine hook returns — it blocks until
        the tests decide she is done eating. After that she is "ready to
        leave", and hooks.leave(sid) may only fire when the etiquette rule
        allows.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("39_dining_hall: DiningHall.student")
