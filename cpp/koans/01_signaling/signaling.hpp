// Koan 01 — Signaling (starter code). Edit this file only.
//
// Guarantee: b1 never runs before a1 has completed, no matter how the
// scheduler interleaves the two threads.
#pragma once

#include <functional>
#include <semaphore>

#include "koans.hpp"

class Signaling {
  public:
    // Thread A's body: run a1(), then let thread B proceed.
    // A must never block waiting for B.
    void run_a(const std::function<void()>& a1) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)a1;
        throw koans::NotImplemented{"Signaling::run_a"};
    }

    // Thread B's body: run b1(), but only after A has finished a1().
    // If B arrives first it must block (not spin) until A signals.
    void run_b(const std::function<void()>& b1) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)b1;
        throw koans::NotImplemented{"Signaling::run_b"};
    }

  private:
    // TODO: your semaphore(s) here. Syntax reminder: the initial value goes
    // in the constructor argument, e.g.  std::counting_semaphore<> sem{N};
    // — choosing N wisely is the koan.
};
