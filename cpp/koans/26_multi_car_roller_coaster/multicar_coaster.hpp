// Koan 26 — Multi-car roller coaster (starter code). Edit this file only.
//
// Guarantee: everything koan 25 promised, with several cars sharing one
// track. Only one car boards passengers at a time; cars take the loading
// dock in fixed rotation 0, 1, ..., n_cars-1, 0, ...; and because cars
// cannot overtake on the track, they unload in the same order they
// loaded. Boards of consecutive carloads never interleave.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

// Injected by the tests. load/run/unload take the car id;
// board/unboard take the passenger id.
struct MultiCarHooks {
    std::function<void(int)> load;
    std::function<void(int)> run;
    std::function<void(int)> unload;
    std::function<void(int)> board;
    std::function<void(int)> unboard;
};

class MultiCarCoaster {
  public:
    MultiCarCoaster(int n_cars, int capacity, MultiCarHooks hooks)
        : n_cars_(n_cars), capacity_(capacity), hooks_(std::move(hooks)) {}

    // One passenger takes one complete ride in whichever car loads: wait
    // until boarding is allowed, call hooks_.board(pid), ride, wait until
    // unboarding is allowed, call hooks_.unboard(pid), then return.
    void passenger(int pid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)pid;
        throw koans::NotImplemented{"MultiCarCoaster::passenger"};
    }

    // Run all n_cars cars concurrently; return when all have finished.
    // Give each car its own thread. Car i performs n_rides_per_car cycles
    // of load/run/unload under the constraints above (loading in rotation
    // starting with car 0, unloading in loading order).
    void start_cars(int n_rides_per_car) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)n_rides_per_car;
        throw koans::NotImplemented{"MultiCarCoaster::start_cars"};
    }

  private:
    int n_cars_;
    int capacity_;
    MultiCarHooks hooks_;
    // TODO: your synchronization members here. Careful: a
    // std::vector<std::binary_semaphore> will not compile — semaphores are
    // neither copyable nor movable. See the README for your options.
};
