"""Koan 16 — Cigarette smokers (starter code). Edit this file only.

Guarantee: each time the agent (played by the test) puts two ingredients
on the table, exactly one smoker — the one who owns the third ingredient —
rolls and smokes exactly one cigarette, then signals the agent. Nobody
else consumes anything, and nothing deadlocks.
"""

import threading

INGREDIENTS = ("tobacco", "paper", "match")


class AgentTable:
    """The agent's semaphores (provided — do not modify).

    The test plays the agent, per the classic rules: agent code is
    untouchable. Each round the agent waits on ``agent_sem``, then releases
    two of the three ingredient semaphores. Your threads may acquire the
    ingredient semaphores and release ``agent_sem`` — never the other way
    around. (``match_`` has a trailing underscore to mirror the C++ field.)
    """

    def __init__(self):
        self.agent_sem = threading.Semaphore(1)
        self.tobacco = threading.Semaphore(0)
        self.paper = threading.Semaphore(0)
        self.match_ = threading.Semaphore(0)

    def ingredient_sem(self, kind):
        """Semaphore for one ingredient name (provided — free to use)."""
        return self.match_ if kind == "match" else getattr(self, kind)


class Smokers:
    """The three smokers (and whatever helpers they need) as daemon threads."""

    def __init__(self, table, on_smoke):
        self.table = table
        self.on_smoke = on_smoke  # call as on_smoke(kind) when a smoker smokes
        # TODO: shared state for your threads goes here.

    def start(self):
        """Spawn your daemon threads; return immediately.

        Each time the smoker who owns ``kind`` rolls and smokes, call
        ``self.on_smoke(kind)`` and then release ``self.table.agent_sem``
        so the agent can serve the next round.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("16_cigarette_smokers: Smokers.start")
