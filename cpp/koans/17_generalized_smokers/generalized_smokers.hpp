// Koan 17 — Generalized smokers (starter code). Edit this file only.
//
// Guarantee: the agent now fires without waiting, so ingredient pairs can
// land in bursts and duplicates can pile up on the table. Every ingredient
// released must eventually be consumed by the one smoker who can complete
// it into a cigarette — none lost, none conjured — no matter how the
// releases interleave.
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>

#include "koans.hpp"

// The generalized agent's semaphores (provided — do not modify).
//
// Unlike koan 16, this agent never waits its turn: agent_sem starts at 0
// and is never used by anyone. The test blasts ingredient pairs
// back-to-back, so several tokens of the SAME ingredient may be pending
// at once.
struct AgentTable {
    std::counting_semaphore<> agent_sem{0};  // unused: nobody waits, ever
    std::counting_semaphore<> tobacco{0};
    std::counting_semaphore<> paper{0};
    std::counting_semaphore<> match_{0};

    // Semaphore for an ingredient index 0=tobacco, 1=paper, 2=match
    // (provided — free to use).
    std::counting_semaphore<>& ingredient_sem(int kind) {
        switch (kind) {
            case 0: return tobacco;
            case 1: return paper;
            default: return match_;
        }
    }
};

inline const char* kIngredients[3] = {"tobacco", "paper", "match"};

// The three smokers (and whatever helpers they need) as detached threads.
class GeneralizedSmokers {
  public:
    GeneralizedSmokers(std::shared_ptr<AgentTable> table,
                       std::function<void(const std::string&)> on_smoke)
        : table_(std::move(table)), on_smoke_(std::move(on_smoke)) {}

    // Spawn your detached threads; return immediately.
    //
    // Each time the smoker who owns ingredient `kind` rolls and smokes,
    // call on_smoke(kind). There is no agent to signal this time.
    //
    // Detached threads outlive this object and the test: every scrap of
    // state they touch must be owned by a shared_ptr they capture by
    // value (the table_ member is already a shared_ptr for this reason).
    void start() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"GeneralizedSmokers::start"};
    }

  private:
    std::shared_ptr<AgentTable> table_;
    std::function<void(const std::string&)> on_smoke_;
    // TODO: shared state for your threads goes here (shared_ptr-owned!).
};
