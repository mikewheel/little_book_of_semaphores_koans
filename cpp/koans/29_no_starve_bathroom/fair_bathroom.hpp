// Koan 29 — No-starve unisex bathroom (starter code). Edit this file only.
//
// Guarantees: men and women are never inside together; at most `capacity`
// people inside; same gender shares the room; and nobody starves —
// opposite-gender arrivals that show up after someone is already waiting
// cannot get in ahead of them.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class FairBathroom {
  public:
    explicit FairBathroom(int capacity = 3) : capacity_(capacity) {
        // TODO: initialize your sync members.
    }

    // Block until this man may enter without breaking the rules.
    void male_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"FairBathroom::male_enter"};
    }

    // Leave the bathroom (caller entered via male_enter).
    void male_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"FairBathroom::male_exit"};
    }

    // Block until this woman may enter without breaking the rules.
    void female_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"FairBathroom::female_enter"};
    }

    // Leave the bathroom (caller entered via female_enter).
    void female_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"FairBathroom::female_exit"};
    }

  private:
    int capacity_;
    // TODO: your sync members here.
};
