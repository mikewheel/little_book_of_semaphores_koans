// Koan 28 — Unisex bathroom (starter code). Edit this file only.
//
// Guarantee: the two genders are never inside at the same time; at most
// `capacity` people are inside at once; and up to `capacity` people of
// the same gender can share. (Starvation is allowed in this koan — a
// stream of one gender may shut the other out indefinitely. Koan 29
// fixes that.)
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class Bathroom {
  public:
    explicit Bathroom(int capacity = 3) : capacity_(capacity) {}

    // Block until entering is allowed: no men inside, and fewer than
    // capacity_ women inside.
    void female_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Bathroom::female_enter"};
    }

    void female_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Bathroom::female_exit"};
    }

    // Block until entering is allowed: no women inside, and fewer than
    // capacity_ men inside.
    void male_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Bathroom::male_enter"};
    }

    void male_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Bathroom::male_exit"};
    }

  private:
    int capacity_;
    // TODO: your synchronization members here.
};
