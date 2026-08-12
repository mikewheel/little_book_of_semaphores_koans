// Koan 27 — Search-Insert-Delete (starter code). Edit this file only.
//
// Guarantee: three-way categorical exclusion around a shared linked list.
// Any number of searchers may run together; at most one inserter runs at
// a time but it may overlap with searchers; a deleter runs completely
// alone — no searchers, no inserters, no other deleters.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class ListGuard {
  public:
    // Block until searching is allowed (i.e. no deleter is inside).
    void search_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::search_enter"};
    }

    // Leave the list; possibly the deleter's cue.
    void search_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::search_exit"};
    }

    // Block until inserting is allowed: no deleter inside and no other
    // inserter inside. Searchers are fine.
    void insert_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::insert_enter"};
    }

    void insert_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::insert_exit"};
    }

    // Block until this deleter is completely alone in the list.
    void delete_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::delete_enter"};
    }

    void delete_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ListGuard::delete_exit"};
    }

  private:
    // TODO: your synchronization members here.
};
