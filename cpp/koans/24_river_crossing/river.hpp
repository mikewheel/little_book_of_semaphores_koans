// Koan 24 — River crossing (starter code). Edit this file only.
//
// Guarantee: threads cross the river only in legal boatloads of exactly
// four — four hackers, four serfs, or two of each. All four board() calls
// of a boatload happen before any board() of the next boatload, and
// exactly one passenger per boatload calls row_boat() after everyone has
// boarded.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>
#include <string>

#include "koans.hpp"

// Injected by the tests. board(kind) as a passenger boards; row_boat(kind)
// by the one rower per boatload. kind is "hacker" or "serf".
struct BoatHooks {
    std::function<void(const std::string&)> board;
    std::function<void(const std::string&)> row_boat;
};

class Boat {
  public:
    explicit Boat(BoatHooks hooks) : hooks_(std::move(hooks)) {}

    // One hacker reaches the dock. Block until this thread belongs to a
    // legal boatload of four, call hooks_.board("hacker"), and — if this
    // thread ends up rowing — call hooks_.row_boat("hacker") once all four
    // have boarded. Return only when this boatload has sailed.
    void hacker_arrives() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Boat::hacker_arrives"};
    }

    // One serf reaches the dock. Same contract, kind "serf".
    void serf_arrives() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Boat::serf_arrives"};
    }

  private:
    BoatHooks hooks_;
    // TODO: your synchronization members here.
};
