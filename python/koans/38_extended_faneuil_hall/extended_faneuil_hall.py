"""Koan 38 — Extended Faneuil Hall (starter code). Edit this file only.

Everything from koan 37 still holds: no entries and no immigrant exits
while the judge is inside; confirm only after every entered immigrant has
checked in; certificates only after confirm. New guarantee: once the judge
has left, every immigrant sworn in at that ceremony must be out of the
building before the judge's next enter may fire.

Hooks may block (the tests hold them open to stage scenarios); your
synchronization must stay correct while they do.
"""

import threading


class ExtendedFaneuilHall:
    def __init__(self, hooks):
        self.hooks = hooks
        # TODO: your synchronization members.

    def immigrant(self, iid):
        """One immigrant: hooks.enter("immigrant:<iid>"), hooks.check_in(iid),
        hooks.sit_down(iid), hooks.swear(iid), hooks.get_certificate(iid),
        hooks.leave("immigrant:<iid>") — each only when the rules allow it.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "38_extended_faneuil_hall: ExtendedFaneuilHall.immigrant"
        )

    def spectator(self, sid):
        """One spectator: hooks.enter("spectator:<sid>"), hooks.spectate(sid),
        hooks.leave("spectator:<sid>"). Spectators may leave at any time."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "38_extended_faneuil_hall: ExtendedFaneuilHall.spectator"
        )

    def judge_visit(self):
        """One complete visit: hooks.enter("judge"), hooks.confirm(),
        hooks.leave("judge").

        May be called repeatedly — even while a previous visit is still
        wrapping up; the new visit's enter must simply wait its turn. A
        visit that finds no immigrants must still complete.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "38_extended_faneuil_hall: ExtendedFaneuilHall.judge_visit"
        )
