// Koan 07 — Queue (starter code). Edit this file only.
//
// Guarantee: dancers proceed only in leader/follower pairs. An arriving
// leader blocks until a follower is (or becomes) available, and vice
// versa.
#pragma once

#include <semaphore>

#include "koans.hpp"

class DanceFloor {
  public:
    // Block until this leader has been matched with a follower.
    void leader_arrives() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"DanceFloor::leader_arrives"};
    }

    // Block until this follower has been matched with a leader.
    void follower_arrives() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"DanceFloor::follower_arrives"};
    }

  private:
    // TODO: your semaphores here (initial values matter!).
};
