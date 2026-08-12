// Koan 23 — Building H2O (starter code). Edit this file only.
//
// Guarantee: atoms pass the assembly point only as complete molecules. Cut
// the sequence of bond() calls into consecutive groups of three: every
// group contains exactly two "H" bonds and one "O" bond, and no atom
// returns until all three atoms of its molecule have bonded.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>
#include <string>

#include "koans.hpp"

// Injected by the tests: call hooks_.bond("H") / hooks_.bond("O") as an
// atom commits to the current molecule.
struct H2OHooks {
    std::function<void(const std::string&)> bond;
};

class H2OBarrier {
  public:
    explicit H2OBarrier(H2OHooks hooks) : hooks_(std::move(hooks)) {}

    // One hydrogen atom arrives. Block until this thread can be one of the
    // two hydrogens of a molecule (together with one more H and one O),
    // call hooks_.bond("H"), and return only after all three atoms of this
    // molecule have bonded.
    void hydrogen() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"H2OBarrier::hydrogen"};
    }

    // One oxygen atom arrives. Block until two hydrogens are ready to join
    // this oxygen, call hooks_.bond("O"), and return only after all three
    // atoms of this molecule have bonded.
    void oxygen() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"H2OBarrier::oxygen"};
    }

  private:
    H2OHooks hooks_;
    // TODO: your synchronization members here.
};
