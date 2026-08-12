// Koan 22 — Santa Claus (starter code). Edit this file only.
//
// Guarantees: Santa acts only when the last reindeer is home (one
// prepare_sleigh, then all n_reindeer get hitched) or when a full group
// of elf_group elves needs help (one help_elves, then exactly those elves
// get_help); while a group is being helped, later elves must wait for a
// whole new group to form.
#pragma once

#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

// The observation seams the tests inject. provided — do not modify.
struct NorthPoleHooks {
    std::function<void()> prepare_sleigh = [] {};
    std::function<void(int)> get_hitched = [](int) {};
    std::function<void()> help_elves = [] {};
    std::function<void(int)> get_help = [](int) {};
};

class NorthPole {
  public:
    explicit NorthPole(NorthPoleHooks hooks, int n_reindeer = 9,
                       int elf_group = 3)
        : hooks_(std::move(hooks)),
          n_reindeer_(n_reindeer),
          elf_group_(elf_group) {
        // TODO: initialize the synchronization members you add below.
    }

    // Spawn Santa as a detached daemon thread. Santa sleeps until woken;
    // on each wake he either preps the sleigh (hooks_.prepare_sleigh(),
    // then lets all n_reindeer get hitched) or helps a waiting group of
    // elves (hooks_.help_elves(), then lets exactly that group get help).
    // Reindeer take priority when both are ready. He loops forever —
    // many flights, many elf groups.
    void start_santa() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NorthPole::start_santa"};
    }

    // A reindeer comes home. The last arrival wakes Santa. Blocks until
    // Santa has prepped the sleigh, then calls hooks_.get_hitched(rid)
    // and returns.
    void reindeer_arrives(int rid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)rid;
        throw koans::NotImplemented{"NorthPole::reindeer_arrives"};
    }

    // An elf hits a problem. Elves gather in groups of elf_group; the
    // group's last member wakes Santa. Blocks until Santa helps the
    // group, then calls hooks_.get_help(eid) and returns. No new elf may
    // start forming a group while a group is being helped.
    void elf_needs_help(int eid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)eid;
        throw koans::NotImplemented{"NorthPole::elf_needs_help"};
    }

  private:
    NorthPoleHooks hooks_;
    int n_reindeer_;
    int elf_group_;
    // TODO: your synchronization members here.
};
