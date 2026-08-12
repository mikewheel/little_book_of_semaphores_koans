// Koan 06 — Reusable barrier (starter code). Edit this file only.
//
// Guarantee: threads call wait() in a loop. In every round, no thread
// returns from wait() until all n threads have entered it that round, and
// no thread can start the next round's wait() while a straggler is still
// leaving this one (no lapping).
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class ReusableBarrier {
  public:
    explicit ReusableBarrier(int n) : n_(n) {}

    // Arrival phase: block until all n threads have called this.
    void phase1() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReusableBarrier::phase1"};
    }

    // Departure phase: block until all n threads are clear to loop.
    void phase2() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReusableBarrier::phase2"};
    }

    // Arrive at the barrier; return when the whole cohort may proceed.
    void wait() {
        phase1();
        phase2();
    }

  private:
    int n_;
    // TODO: a counter, something to protect it, and whatever the two
    // phases need to sleep on.
};
