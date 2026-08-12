"""Koan 37 — Faneuil Hall (starter code). Edit this file only.

Guarantees being built: while the judge is in the building nobody enters
and no immigrant leaves (spectators may); the judge confirms only after
every immigrant who entered has checked in; certificates are handed out
only after the confirmation.

The hooks object is the ceremony itself: each hook call IS the action, so
the ordering rules are about when the hooks fire. Hooks may block — the
tests hold them open to stage scenarios — and your synchronization must
stay correct while they do.
"""

import threading


class FaneuilHall:
    def __init__(self, hooks):
        self.hooks = hooks
        # TODO: your synchronization members.

    def immigrant(self, iid):
        """One immigrant, start to finish.

        Calls, in order: hooks.enter("immigrant:<iid>"), hooks.check_in(iid),
        hooks.sit_down(iid), hooks.swear(iid), hooks.get_certificate(iid),
        hooks.leave("immigrant:<iid>") — each only when the rules allow it.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("37_faneuil_hall: FaneuilHall.immigrant")

    def spectator(self, sid):
        """One spectator: hooks.enter("spectator:<sid>"), hooks.spectate(sid),
        hooks.leave("spectator:<sid>"). Spectators may leave at any time."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("37_faneuil_hall: FaneuilHall.spectator")

    def judge_visit(self):
        """One complete visit by the judge: hooks.enter("judge"),
        hooks.confirm(), hooks.leave("judge").

        May be called repeatedly — each call is a fresh ceremony, and a
        visit that finds no immigrants must still complete.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("37_faneuil_hall: FaneuilHall.judge_visit")
