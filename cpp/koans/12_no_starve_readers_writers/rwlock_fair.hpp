// Koan 12 — No-starve readers-writers (starter code). Edit this file only.
//
// Guarantees: koan 11's safety rules (readers share; a writer is alone),
// plus fairness for writers: readers arriving AFTER a waiting writer do not
// enter before it. Incumbent readers finish; the writer goes next.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class NoStarveReadWriteLock {
  public:
    // Block while a writer is inside OR waiting; readers still share.
    void reader_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveReadWriteLock::reader_enter"};
    }

    // Leave the room. The last reader out has a special job.
    void reader_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveReadWriteLock::reader_exit"};
    }

    // Queue up, bar later arrivals, wait for the room to empty.
    void writer_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveReadWriteLock::writer_enter"};
    }

    // Give up the room and reopen the doorway.
    void writer_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveReadWriteLock::writer_exit"};
    }

  private:
    // TODO: koan 11's members, plus a doorway that a waiting writer can
    // hold shut.
};
