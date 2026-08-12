// Koan 05 — Barrier (starter code). Edit this file only.
//
// Guarantee: no call to wait() returns until all n threads have called it.
// One-shot: the barrier only needs to work for a single batch.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class Barrier {
  public:
    explicit Barrier(int n) : n_(n) {}

    // Block until n threads (including this one) have called wait().
    void wait() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Barrier::wait"};
    }

  private:
    int n_;
    // TODO: a counter, something to protect it, and something for early
    // arrivals to sleep on.
};
