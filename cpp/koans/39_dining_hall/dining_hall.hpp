// Koan 39 — Dining Hall (starter code). Edit this file only.
//
// Guarantee being built: no student is ever stranded eating alone. A
// student who has finished dining may only fire her leave hook if doing so
// would not leave exactly one other student still eating with nobody else
// about to go; the stranded situation resolves when a newcomer starts
// dining or the lone diner finishes (the two then leave together).
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>

#include "koans.hpp"

// Provided — do not modify.
struct DiningHooks {
    std::function<void(int)> dine;
    std::function<void(int)> leave;
};

class DiningHall {
  public:
    explicit DiningHall(DiningHooks hooks) : hooks_(std::move(hooks)) {
        // TODO: initialize your synchronization members.
    }

    // One student's meal, start to finish. Calls hooks_.dine(sid) when she
    // sits down to eat; if dine_gate is non-empty, call it after the dine
    // hook returns — it blocks until the tests decide she is done eating.
    // After that she is "ready to leave", and hooks_.leave(sid) may only
    // fire when the etiquette rule allows.
    void student(int sid, const std::function<void()>& dine_gate = {}) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)sid;
        (void)dine_gate;
        throw koans::NotImplemented{"DiningHall::student"};
    }

  private:
    DiningHooks hooks_;
    // TODO: your counters / lock / semaphore here.
};
