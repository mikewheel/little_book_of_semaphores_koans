// Koan 31 — The Modus Hall problem (starter code). Edit this file only.
//
// Guarantees: heathens and prudes are never on the path together; a
// faction shares the path freely with itself; an empty path goes to the
// first arrival; and control flips by majority rule — when the queued
// opposition outnumbers the current holders, new holders are barred,
// incumbents finish, and the whole waiting cohort crosses. A minority
// keeps waiting.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

class Path {
  public:
    Path() {
        // TODO: initialize your sync members.
    }

    // Arrive as a heathen; wait if required; run cross() while on the path.
    void heathen_cross(const std::function<void()>& cross) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)cross;
        throw koans::NotImplemented{"Path::heathen_cross"};
    }

    // Arrive as a prude; wait if required; run cross() while on the path.
    void prude_cross(const std::function<void()>& cross) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)cross;
        throw koans::NotImplemented{"Path::prude_cross"};
    }

  private:
    // TODO: your scoreboard and sync members here.
};
