// Koan 41 — Build a semaphore (starter code). Edit this file only.
//
// Guarantees being built (the semaphore properties, from the book):
//  1. acquire() blocks while the value is exhausted and proceeds otherwise;
//  2. release() banks a permit that a future acquire() can spend — signals
//     are never lost, even with nobody waiting yet;
//  3. when a release() wakes the waiters, one of the threads that was
//     actually waiting gets in — a fresh caller racing in cannot snatch
//     that wakeup.
//
// House rule (the whole point of the koan): build it from std::mutex and
// std::condition_variable only — do not include <semaphore>. This header's
// includes are already exactly what you are allowed to use.
#pragma once

#include <condition_variable>
#include <mutex>

#include "koans.hpp"

class HandmadeSemaphore {
  public:
    // Initial value must be nonnegative (the tests never pass less).
    explicit HandmadeSemaphore(int value = 0) : value_(value) {
        // TODO: any further setup for your machinery (and one more counter
        // — see the hints if the third guarantee gets slippery).
    }

    // Take a permit, blocking until one is available.
    void acquire() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"HandmadeSemaphore::acquire"};
    }

    // Bank one permit and, if anyone is waiting, wake exactly one.
    void release() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"HandmadeSemaphore::release"};
    }

  private:
    int value_;
    // TODO: your mutex / condition_variable (and more?) here.
};
