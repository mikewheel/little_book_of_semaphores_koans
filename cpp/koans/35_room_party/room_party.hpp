// Koan 35 — Room party (starter code). Edit this file only.
//
// Guarantees: the dean enters only an empty room (search) or an
// over-threshold party (breakup), waiting outside otherwise; while the
// dean is inside no student enters but students may leave; after a
// breakup the dean stays until the room is empty. One dean, any number
// of students.
#pragma once

#include <functional>
#include <semaphore>

#include "koans.hpp"

class Room {
  public:
    explicit Room(int threshold = 50,
                  std::function<void()> search = {},
                  std::function<void()> breakup = {},
                  std::function<void(int)> party = {})
        : threshold_(threshold),
          // Test-supplied hooks (already wired — leave these three lines).
          search_(search ? std::move(search) : [] {}),
          breakup_(breakup ? std::move(breakup) : [] {}),
          party_(party ? std::move(party) : [](int) {}) {
        // TODO: your synchronization state here.
        (void)threshold_;
    }

    // Enter (waiting out the dean), party(sid), then leave.
    void student_visit(int sid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)sid;
        throw koans::NotImplemented{"Room::student_visit"};
    }

    // Search an empty room, or break up a big party and hold the door
    // until it empties; with 1..threshold students inside, wait for one
    // of those conditions. Returns when the dean leaves.
    void dean_visit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Room::dean_visit"};
    }

  private:
    int threshold_;
    std::function<void()> search_;
    std::function<void()> breakup_;
    std::function<void(int)> party_;
};
