// Koan 40 — Extended Dining Hall (starter code). Edit this file only.
//
// Guarantees being built: nobody eats alone and nobody is left eating
// alone. A student may not START dining while the table is empty and no
// other student is ready to eat (she waits for company; the pair sits down
// together). And, as in koan 39, a finished student may not leave if that
// would strand exactly one diner with no other leaver for company.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

// Provided — do not modify.
struct DiningHooks {
    std::function<void(int)> get_food;
    std::function<void(int)> dine;
    std::function<void(int)> leave;
};

class ExtendedDiningHall {
  public:
    explicit ExtendedDiningHall(DiningHooks hooks) : hooks_(std::move(hooks)) {
        // TODO: initialize your synchronization members.
    }

    // One student's meal: get food, dine, leave. Calls hooks_.get_food(sid)
    // first — after it returns she is "ready to eat". hooks_.dine(sid) may
    // only fire when the sitting-down rule allows. If dine_gate is
    // non-empty, call it after the dine hook returns — it blocks until the
    // tests decide she is done eating. Then she is "ready to leave", and
    // hooks_.leave(sid) may only fire when the leaving rule allows.
    void student(int sid, const std::function<void()>& dine_gate = {}) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)sid;
        (void)dine_gate;
        throw koans::NotImplemented{"ExtendedDiningHall::student"};
    }

  private:
    DiningHooks hooks_;
    // TODO: your counters / lock / semaphores here.
};
