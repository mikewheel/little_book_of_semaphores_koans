// Koan 30 — Baboon crossing (starter code). Edit this file only.
//
// Guarantees: eastbound and westbound baboons are never on the rope at
// the same time; at most `capacity` baboons on the rope; same-direction
// baboons share the rope up to capacity; and no direction can starve the
// other — later opposing arrivals cannot overtake a baboon that is
// already waiting.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class Rope {
  public:
    explicit Rope(int capacity = 5) : capacity_(capacity) {
        // TODO: initialize your sync members.
    }

    // Block until this eastbound baboon may get on the rope.
    void east_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Rope::east_enter"};
    }

    // Step off the rope (caller entered via east_enter).
    void east_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Rope::east_exit"};
    }

    // Block until this westbound baboon may get on the rope.
    void west_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Rope::west_enter"};
    }

    // Step off the rope (caller entered via west_enter).
    void west_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Rope::west_exit"};
    }

  private:
    int capacity_;
    // TODO: your sync members here.
};
