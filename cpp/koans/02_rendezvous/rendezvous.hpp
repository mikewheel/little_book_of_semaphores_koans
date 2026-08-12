// Koan 02 — Rendezvous (starter code). Edit this file only.
//
// Guarantees: a1 happens before b2, and b1 happens before a2. The order of
// a1 vs b1 must remain unconstrained.
#pragma once

#include <functional>
#include <semaphore>

#include "koans.hpp"

class Rendezvous {
  public:
    // Thread A's body: a1(), meet B, then a2().
    void run_a(const std::function<void()>& a1,
               const std::function<void()>& a2) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)a1;
        (void)a2;
        throw koans::NotImplemented{"Rendezvous::run_a"};
    }

    // Thread B's body: b1(), meet A, then b2().
    void run_b(const std::function<void()>& b1,
               const std::function<void()>& b2) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)b1;
        (void)b2;
        throw koans::NotImplemented{"Rendezvous::run_b"};
    }

  private:
    // TODO: your semaphores here (initial values matter!).
};
