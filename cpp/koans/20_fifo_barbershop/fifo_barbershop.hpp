// Koan 20 — FIFO Barbershop (starter code). Edit this file only.
//
// Guarantees: everything koan 19 promised — capacity n with balking, a
// sleeping barber, 1:1 fully-finished haircuts — PLUS customers are served
// in the order they arrived (arrival = the moment customer_visit registers
// them, inside its mutual exclusion).
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

class FifoBarbershop {
  public:
    // n = max customers in the shop (waiting room + chair).
    explicit FifoBarbershop(int n) : n_(n) {
        // TODO: initialize the synchronization members you add below.
    }

    // Spawn the barber as a detached daemon thread. Same as koan 19, with
    // one addition: the barber must serve waiting customers strictly in
    // their arrival order.
    void start_barber(std::function<void()> cut_hair) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)cut_hair;
        throw koans::NotImplemented{"FifoBarbershop::start_barber"};
    }

    // One customer's trip to the shop. Balk with an immediate false if the
    // shop holds n customers. Otherwise register your arrival, wait until
    // the barber calls *you* (not just anyone), run get_hair_cut()
    // concurrently with cut_hair(), and return true once the cut is fully
    // done.
    bool customer_visit(const std::function<void()>& get_hair_cut) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)get_hair_cut;
        throw koans::NotImplemented{"FifoBarbershop::customer_visit"};
    }

  private:
    int n_;
    // TODO: your synchronization members here.
};
