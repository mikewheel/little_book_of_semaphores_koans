"""Koan 17 — Generalized smokers (starter code). Edit this file only.

Guarantee: the agent now fires without waiting, so ingredient pairs can
land in bursts and duplicates can pile up on the table. Every ingredient
released must eventually be consumed by the one smoker who can complete
it into a cigarette — none lost, none conjured — no matter how the
releases interleave.
"""

import threading

INGREDIENTS = ("tobacco", "paper", "match")


class AgentTable:
    """The generalized agent's semaphores (provided — do not modify).

    Unlike koan 16, this agent never waits its turn: ``agent_sem`` starts
    at 0 and is never used by anyone. The test blasts ingredient pairs
    back-to-back, so several tokens of the SAME ingredient may be pending
    at once.
    """

    def __init__(self):
        self.agent_sem = threading.Semaphore(0)  # unused: nobody waits, ever
        self.tobacco = threading.Semaphore(0)
        self.paper = threading.Semaphore(0)
        self.match_ = threading.Semaphore(0)

    def ingredient_sem(self, kind):
        """Semaphore for one ingredient name (provided — free to use)."""
        return self.match_ if kind == "match" else getattr(self, kind)


class GeneralizedSmokers:
    """The three smokers (and whatever helpers they need) as daemon threads."""

    def __init__(self, table, on_smoke):
        self.table = table
        self.on_smoke = on_smoke  # call as on_smoke(kind) when a smoker smokes
        # TODO: shared state for your threads goes here.

    def start(self):
        """Spawn your daemon threads; return immediately.

        Each time the smoker who owns ``kind`` rolls and smokes, call
        ``self.on_smoke(kind)``. There is no agent to signal this time.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError(
            "17_generalized_smokers: GeneralizedSmokers.start"
        )
