// Koan 18 — Dining Savages (starter code). Edit this file only.
//
// Guarantees: nobody takes a serving from an empty pot, the cook refills
// only a pot that is truly empty, and the cook sleeps until a diner wakes
// him. The pot object itself is NOT thread-safe — keeping every pot call
// exclusive is part of your job.
#pragma once

#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

// The interface the tests' instrumented pot implements.
// provided — do not modify.
class Pot {
  public:
    virtual ~Pot() = default;
    virtual void put_servings(int m) = 0;  // refill the empty pot with m
    virtual void get_serving() = 0;        // remove exactly one serving
};

class Village {
  public:
    // m = servings added per refill. The pot outlives the Village.
    Village(int m, Pot& pot) : m_(m), pot_(pot) {
        // TODO: initialize the synchronization members you add below.
    }

    // Spawn the cook as a detached daemon thread. It sleeps until a diner
    // reports the pot empty, calls pot.put_servings(m), announces the
    // refill, and loops forever.
    void start_cook() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Village::start_cook"};
    }

    // Take exactly one serving via pot.get_serving(). If the pot is empty,
    // wake the cook and wait for the refill before taking it.
    void dine() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Village::dine"};
    }

  private:
    int m_;
    Pot& pot_;
    // TODO: your synchronization members here.
};
