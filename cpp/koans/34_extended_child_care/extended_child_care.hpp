// Koan 34 — Extended child care (starter code). Edit this file only.
//
// Guarantees: at every moment, children inside <= ratio x adults inside
// (an adult waiting to leave still counts as inside), and nobody waits
// unnecessarily — a blocked adult_leave must not keep out a child the
// ratio genuinely allows, and a waiting adult departs as soon as the
// counts permit. adult_enter and child_leave never block.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class ExtendedChildCare {
  public:
    explicit ExtendedChildCare(int ratio = 3) : ratio_(ratio) {
        // TODO: your synchronization state here.
        (void)ratio_;
    }

    // An adult walks in. Never blocks.
    void adult_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ExtendedChildCare::adult_enter"};
    }

    // An adult walks out the moment the invariant survives it.
    void adult_leave() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ExtendedChildCare::adult_leave"};
    }

    // A child comes in, waiting only while the ratio truly forbids it.
    void child_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ExtendedChildCare::child_enter"};
    }

    // A child goes home. Never blocks.
    void child_leave() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ExtendedChildCare::child_leave"};
    }

  private:
    int ratio_;
};
