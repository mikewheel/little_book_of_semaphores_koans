// Koan 32 — The sushi bar problem (starter code). Edit this file only.
//
// Guarantees: at most `seats` customers eat at once; arrivals seat
// immediately while a seat is free and the bar has not filled; once the
// bar fills, later arrivals wait until it is COMPLETELY empty, then the
// waiting cohort (up to `seats`) is seated together.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

class SushiBar {
  public:
    explicit SushiBar(int seats = 5) : seats_(seats) {
        // TODO: initialize your scoreboard and sync members.
    }

    // Arrive; wait if the rules demand it; run eat() while seated.
    void dine(const std::function<void()>& eat) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)eat;
        throw koans::NotImplemented{"SushiBar::dine"};
    }

  private:
    int seats_;
    // TODO: your scoreboard and sync members here.
};
