// Koan 03 — Mutex (starter code). Edit this file only.
//
// Guarantee: between acquire() and release(), no other thread is between
// its own acquire() and release(). Works for any number of threads.
#pragma once

#include <semaphore>

#include "koans.hpp"

class Mutex {
  public:
    // Block until the critical section is free, then claim it.
    void acquire() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Mutex::acquire"};
    }

    // Leave the critical section, admitting one waiter (if any).
    void release() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Mutex::release"};
    }

  private:
    // TODO: your semaphore here. The initial value is the whole game.
};
