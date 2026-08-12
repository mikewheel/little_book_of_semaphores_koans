// Koan 16 — Cigarette smokers (starter code). Edit this file only.
//
// Guarantee: each time the agent (played by the test) puts two ingredients
// on the table, exactly one smoker — the one who owns the third
// ingredient — rolls and smokes exactly one cigarette, then signals the
// agent. Nobody else consumes anything, and nothing deadlocks.
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>

#include "koans.hpp"

// The agent's semaphores (provided — do not modify).
//
// The test plays the agent, per the classic rules: agent code is
// untouchable. Each round the agent waits on agent_sem, then releases two
// of the three ingredient semaphores. Your threads may acquire the
// ingredient semaphores and release agent_sem — never the other way
// around. (match_ has a trailing underscore: `match` is asking for
// trouble as an identifier.)
struct AgentTable {
    std::counting_semaphore<> agent_sem{1};
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
class Smokers {
  public:
    Smokers(std::shared_ptr<AgentTable> table,
            std::function<void(const std::string&)> on_smoke)
        : table_(std::move(table)), on_smoke_(std::move(on_smoke)) {}

    // Spawn your detached threads; return immediately.
    //
    // Each time the smoker who owns ingredient `kind` rolls and smokes,
    // call on_smoke(kind) and then release table->agent_sem so the agent
    // can serve the next round.
    //
    // Detached threads outlive this object and the test: every scrap of
    // state they touch must be owned by a shared_ptr they capture by
    // value (the table_ member is already a shared_ptr for this reason).
    void start() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"Smokers::start"};
    }

  private:
    std::shared_ptr<AgentTable> table_;
    std::function<void(const std::string&)> on_smoke_;
    // TODO: shared state for your threads goes here (shared_ptr-owned!).
};
