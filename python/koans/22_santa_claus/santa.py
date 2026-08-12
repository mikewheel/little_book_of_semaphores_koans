"""Koan 22 — Santa Claus (starter code). Edit this file only.

Guarantees: Santa acts only when the last reindeer is home (one
prepare_sleigh, then all n_reindeer get hitched) or when a full group of
elf_group elves needs help (one help_elves, then exactly those elves
get_help); while a group is being helped, later elves must wait for a
whole new group to form.
"""

import threading


class NorthPole:
    def __init__(self, hooks, n_reindeer=9, elf_group=3):
        self.hooks = hooks  # prepare_sleigh(), get_hitched(rid),
        #                     help_elves(), get_help(eid)
        self.n_reindeer = n_reindeer
        self.elf_group = elf_group
        # TODO: the synchronization members you need.

    def start_santa(self):
        """Spawn Santa as a daemon thread.

        Santa sleeps until woken; on each wake he either preps the sleigh
        (hooks.prepare_sleigh(), then lets all n_reindeer get hitched) or
        helps a waiting group of elves (hooks.help_elves(), then lets
        exactly that group get help). Reindeer take priority when both
        are ready. He loops forever — many flights, many elf groups.
        """
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("22_santa_claus: NorthPole.start_santa")

    def reindeer_arrives(self, rid):
        """A reindeer comes home. The last arrival wakes Santa. Blocks
        until Santa has prepped the sleigh, then calls
        hooks.get_hitched(rid) and returns."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("22_santa_claus: NorthPole.reindeer_arrives")

    def elf_needs_help(self, eid):
        """An elf hits a problem. Elves gather in groups of elf_group; the
        group's last member wakes Santa. Blocks until Santa helps the
        group, then calls hooks.get_help(eid) and returns. No new elf may
        start forming a group while a group is being helped."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("22_santa_claus: NorthPole.elf_needs_help")
