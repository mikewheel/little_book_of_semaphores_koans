// Koan 04 — Multiplex (starter code). Edit this file only.
//
// Guarantee: at most n threads are between enter() and exit() at any
// moment, and n threads CAN be inside simultaneously.
#pragma once

#include <semaphore>

#include "koans.hpp"

class Multiplex {
  public:
    explicit Multiplex(int n) {
        // TODO: size the room. (Hint: the constructor argument of a
        // std::counting_semaphore<> is its initial token count.)
        (void)n;
    }

    // Block until the room has capacity, then come in.
    void enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Multiplex::enter"};
    }

    // Leave the room, freeing a slot for one waiter (if any).
    void exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Multiplex::exit"};
    }

  private:
    // TODO: your semaphore here.
};
