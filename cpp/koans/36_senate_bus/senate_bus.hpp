// Koan 36 — Senate bus (starter code). Edit this file only.
//
// Guarantees: when a bus arrives, exactly the riders already waiting
// board (never more than capacity); riders arriving mid-boarding wait
// for the next bus; depart(n) reports exactly how many boarded this
// bus; a bus at an empty stop departs immediately with depart(0).
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

class BusStop {
  public:
    explicit BusStop(int capacity = 50,
                     std::function<void(int)> board = {},
                     std::function<void(int)> depart = {})
        : capacity_(capacity),
          // Test-supplied hooks (already wired — leave these two lines).
          board_(board ? std::move(board) : [](int) {}),
          depart_(depart ? std::move(depart) : [](int) {}) {
        // TODO: your synchronization state here.
        (void)capacity_;
    }

    // Arrive at the stop, wait for a bus, board it. Returns once rider
    // `rid` is aboard.
    void rider(int rid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)rid;
        throw koans::NotImplemented{"BusStop::rider"};
    }

    // One bus visit: board the eligible waiting riders (up to capacity,
    // none that arrived after the bus), depart(n), return.
    void bus_arrives() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"BusStop::bus_arrives"};
    }

  private:
    int capacity_;
    std::function<void(int)> board_;
    std::function<void(int)> depart_;
};
