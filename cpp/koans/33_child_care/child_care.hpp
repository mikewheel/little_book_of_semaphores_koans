// Koan 33 — Child care (starter code). Edit this file only.
//
// Guarantee: at every moment, children inside <= ratio x adults inside.
// child_enter blocks while admitting the child would break the bound;
// adult_leave blocks while leaving would break it. adult_enter and
// child_leave never block.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class ChildCare {
  public:
    explicit ChildCare(int ratio = 3) : ratio_(ratio) {
        // TODO: your synchronization state here.
        (void)ratio_;
    }

    // An adult walks in. Never blocks.
    void adult_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ChildCare::adult_enter"};
    }

    // An adult walks out — but only once the invariant survives it.
    void adult_leave() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ChildCare::adult_leave"};
    }

    // A child comes in, waiting at the door while the center is full.
    void child_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ChildCare::child_enter"};
    }

    // A child goes home. Never blocks.
    void child_leave() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ChildCare::child_leave"};
    }

  private:
    int ratio_;
};
