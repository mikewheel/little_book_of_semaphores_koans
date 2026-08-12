// Koan 19 — Barbershop (starter code). Edit this file only.
//
// Guarantees: at most n customers in the shop (arrivals beyond that balk
// and leave with false); the barber sleeps until a customer is present;
// each cut_hair() is paired with exactly one customer's get_hair_cut(),
// and a customer's visit only succeeds once their cut is fully finished.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

class Barbershop {
  public:
    // n = max customers in the shop (waiting room + chair).
    explicit Barbershop(int n) : n_(n) {
        // TODO: initialize the synchronization members you add below.
    }

    // Spawn the barber as a detached daemon thread. The barber loops
    // forever: sleep until a customer is present, then call cut_hair()
    // exactly once for that customer, and don't move on to the next
    // customer until this one's haircut is fully done.
    void start_barber(std::function<void()> cut_hair) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)cut_hair;
        throw koans::NotImplemented{"Barbershop::start_barber"};
    }

    // One customer's trip to the shop. If the shop already holds n
    // customers, leave immediately and return false (a "balk") — without
    // blocking. Otherwise wait your turn, call get_hair_cut() while the
    // barber runs cut_hair(), and return true only after both sides of
    // the haircut have finished.
    bool customer_visit(const std::function<void()>& get_hair_cut) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)get_hair_cut;
        throw koans::NotImplemented{"Barbershop::customer_visit"};
    }

  private:
    int n_;
    // TODO: your synchronization members here.
};
