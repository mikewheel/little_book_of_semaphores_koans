// Koan 08 — Exclusive queue (starter code). Edit this file only.
//
// Guarantees: dancers pair up leader/follower; at most one pair is on the
// floor at a time (the pair's two dance callbacks overlap; no other dance
// overlaps them); a leader does not return until its partner's dance has
// completed.
#pragma once

#include <functional>
#include <semaphore>

#include "koans.hpp"

class ExclusiveDanceFloor {
  public:
    // Block until paired with a follower, run dance() while the pair has
    // the floor to itself, and return only after the partner's dance has
    // completed.
    void leader_dances(const std::function<void()>& dance) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)dance;
        throw koans::NotImplemented{"ExclusiveDanceFloor::leader_dances"};
    }

    // Block until paired with a leader, then run dance() while the pair
    // has the floor to itself.
    void follower_dances(const std::function<void()>& dance) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)dance;
        throw koans::NotImplemented{"ExclusiveDanceFloor::follower_dances"};
    }

  private:
    // TODO: waiting-dancer counters, something to protect them, and
    // queues to park on. (Careful which guard you pick — see the README.)
};
