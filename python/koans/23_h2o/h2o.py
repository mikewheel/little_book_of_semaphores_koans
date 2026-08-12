"""Koan 23 — Building H2O (starter code). Edit this file only.

Guarantee: atoms pass the assembly point only as complete molecules. Cut
the sequence of bond() calls into consecutive groups of three: every group
contains exactly two "H" bonds and one "O" bond, and no atom returns until
all three atoms of its molecule have bonded.
"""

import threading


class H2OBarrier:
    def __init__(self, hooks):
        # hooks.bond(kind) must be called as an atom bonds; kind is "H"/"O".
        self.hooks = hooks
        # TODO: your synchronization members here.

    def hydrogen(self):
        """One hydrogen atom arrives.

        Block until this thread can be one of the two hydrogens of a
        molecule (together with one more H and one O), call
        self.hooks.bond("H"), and return only after all three atoms of
        this molecule have bonded.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("23_h2o: H2OBarrier.hydrogen")

    def oxygen(self):
        """One oxygen atom arrives.

        Block until two hydrogens are ready to join this oxygen, call
        self.hooks.bond("O"), and return only after all three atoms of
        this molecule have bonded.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("23_h2o: H2OBarrier.oxygen")
