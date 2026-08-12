// Koan 25 — Roller coaster (starter code). Edit this file only.
//
// Guarantee: per ride cycle the car loads, exactly `capacity` passengers
// board, the car runs, the car unloads, and exactly those passengers
// unboard — in that order. Passengers board only between load() and
// run(), the car runs only when full, and the next load() waits until
// every rider from the previous cycle has unboarded.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

// Injected by the tests. load/run/unload are the car's phases;
// board/unboard take the passenger id.
struct CoasterHooks {
    std::function<void()> load;
    std::function<void()> run;
    std::function<void()> unload;
    std::function<void(int)> board;
    std::function<void(int)> unboard;
};

class RollerCoaster {
  public:
    RollerCoaster(int capacity, CoasterHooks hooks)
        : capacity_(capacity), hooks_(std::move(hooks)) {}

    // One passenger takes one complete ride: wait until boarding is
    // allowed, call hooks_.board(pid), ride, wait until unboarding is
    // allowed, call hooks_.unboard(pid), then return.
    void passenger(int pid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)pid;
        throw koans::NotImplemented{"RollerCoaster::passenger"};
    }

    // The car's body — the tests run it on its own thread. Perform
    // n_rides cycles: hooks_.load(), let exactly capacity_ passengers
    // board, hooks_.run() only once the car is full, hooks_.unload(),
    // and start the next cycle only after all capacity_ riders have
    // unboarded. Return when all n_rides cycles are done.
    void start_car(int n_rides) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)n_rides;
        throw koans::NotImplemented{"RollerCoaster::start_car"};
    }

  private:
    int capacity_;
    CoasterHooks hooks_;
    // TODO: your synchronization members here.
};
