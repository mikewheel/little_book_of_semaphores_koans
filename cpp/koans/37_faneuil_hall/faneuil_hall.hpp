// Koan 37 — Faneuil Hall (starter code). Edit this file only.
//
// Guarantees being built: while the judge is in the building nobody enters
// and no immigrant leaves (spectators may); the judge confirms only after
// every immigrant who entered has checked in; certificates are handed out
// only after the confirmation.
//
// The hooks struct is the ceremony itself: each hook call IS the action, so
// the ordering rules are about when the hooks fire. Hooks may block — the
// tests hold them open to stage scenarios — and your synchronization must
// stay correct while they do.
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

class FaneuilHall {
  public:
    explicit FaneuilHall(FaneuilHooks hooks) : hooks_(std::move(hooks)) {
        // TODO: initialize your synchronization members.
    }

    // One immigrant, start to finish. Calls, in order: enter, check_in,
    // sit_down, swear, get_certificate, leave — each only when the rules
    // allow it.
    void immigrant(int iid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)iid;
        throw koans::NotImplemented{"FaneuilHall::immigrant"};
    }

    // One spectator: enter, spectate, leave. Spectators may leave at any
    // time, judge or no judge.
    void spectator(int sid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)sid;
        throw koans::NotImplemented{"FaneuilHall::spectator"};
    }

    // One complete visit by the judge: enter, confirm, leave. May be called
    // repeatedly — each call is a fresh ceremony, and a visit that finds no
    // immigrants must still complete.
    void judge_visit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"FaneuilHall::judge_visit"};
    }

  private:
    FaneuilHooks hooks_;
    // TODO: your semaphores / counters here.
};
