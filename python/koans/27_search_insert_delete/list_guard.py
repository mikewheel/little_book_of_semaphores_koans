"""Koan 27 — Search-Insert-Delete (starter code). Edit this file only.

Guarantee: three-way categorical exclusion around a shared linked list.
Any number of searchers may run together; at most one inserter runs at a
time but it may overlap with searchers; a deleter runs completely alone —
no searchers, no inserters, no other deleters.
"""

import threading


class ListGuard:
    def __init__(self):
        # TODO: your synchronization members here.
        pass

    def search_enter(self):
        """Block until searching is allowed (i.e. no deleter is inside)."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.search_enter")

    def search_exit(self):
        """Leave the list; possibly the deleter's cue."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.search_exit")

    def insert_enter(self):
        """Block until inserting is allowed: no deleter inside and no
        other inserter inside. Searchers are fine."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.insert_enter")

    def insert_exit(self):
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.insert_exit")

    def delete_enter(self):
        """Block until this deleter is completely alone in the list."""
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.delete_enter")

    def delete_exit(self):
        # ── YOUR CODE HERE ────────────────────────────────────────────────
        raise NotImplementedError("27_search_insert_delete: ListGuard.delete_exit")
