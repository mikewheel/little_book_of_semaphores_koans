// Koan 38 — Extended Faneuil Hall (starter code). Edit this file only.
//
// Everything from koan 37 still holds: no entries and no immigrant exits
// while the judge is inside; confirm only after every entered immigrant has
// checked in; certificates only after confirm. New guarantee: once the
// judge has left, every immigrant sworn in at that ceremony must be out of
// the building before the judge's next enter may fire.
//
// Hooks may block (the tests hold them open to stage scenarios); your
// synchronization must stay correct while they do.
#pragma once

#include <functional>
#include <semaphore>
#include <string>

#include "koans.hpp"

// Provided — do not modify. `who` is "immigrant:<id>", "spectator:<id>",
// or "judge".
struct FaneuilHooks {
    std::function<void(const std::string&)> enter;
    std::function<void(int)> check_in;
    std::function<void(int)> sit_down;
    std::function<void(int)> swear;
    std::function<void(int)> get_certificate;
    std::function<void()> confirm;
    std::function<void(int)> spectate;
    std::function<void(const std::string&)> leave;
};

class ExtendedFaneuilHall {
  public:
    explicit ExtendedFaneuilHall(FaneuilHooks hooks) : hooks_(std::move(hooks)) {
        // TODO: initialize your synchronization members.
    }

    // One immigrant: enter, check_in, sit_down, swear, get_certificate,
    // leave — each only when the rules allow it.
    void immigrant(int iid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)iid;
        throw koans::NotImplemented{"ExtendedFaneuilHall::immigrant"};
    }

    // One spectator: enter, spectate, leave. Spectators may leave at any
    // time, judge or no judge.
    void spectator(int sid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)sid;
        throw koans::NotImplemented{"ExtendedFaneuilHall::spectator"};
    }

    // One complete visit: enter, confirm, leave. May be called repeatedly —
    // even while a previous visit is still wrapping up; the new visit's
    // enter must simply wait its turn. A visit that finds no immigrants
    // must still complete.
    void judge_visit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ExtendedFaneuilHall::judge_visit"};
    }

  private:
    FaneuilHooks hooks_;
    // TODO: your semaphores / counters here.
};
