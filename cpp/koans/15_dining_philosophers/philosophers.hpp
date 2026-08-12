// Koan 15 — Dining philosophers (starter code). Edit this file only.
//
// Guarantees: a fork is held by at most one philosopher at a time (so
// neighbors never eat together), no deadlock even when everyone is hungry
// at once, more than one philosopher CAN eat at the same time, and every
// philosopher who keeps trying gets to eat.
#pragma once

#include <memory>
#include <semaphore>
#include <vector>

#include "koans.hpp"

class Table {
  public:
    explicit Table(int n = 5) : n_(n) {
        // TODO: per-fork exclusivity, plus whatever breaks the deadly cycle.
    }

    // Index of philosopher i's left fork (provided — free to use).
    int left(int i) const { return i; }

    // Index of philosopher i's right fork (provided — free to use).
    int right(int i) const { return (i + 1) % n_; }

    // Block until philosopher i holds BOTH adjacent forks.
    void get_forks(int i) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)i;
        throw koans::NotImplemented{"Table::get_forks"};
    }

    // Return philosopher i's two forks to the table.
    void put_forks(int i) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)i;
        throw koans::NotImplemented{"Table::put_forks"};
    }

  private:
    int n_;
    // TODO: your members here.
};
