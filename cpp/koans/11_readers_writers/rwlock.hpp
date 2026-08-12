// Koan 11 — Readers-writers (starter code). Edit this file only.
//
// Guarantees: any number of readers may hold the lock together; a writer
// holds it alone — no readers, no other writers. (Writer starvation is NOT
// addressed here; that's koan 12.)
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class ReadWriteLock {
  public:
    // Block while a writer is inside; readers never block readers.
    void reader_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReadWriteLock::reader_enter"};
    }

    // Leave the room. The last reader out has a special job.
    void reader_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReadWriteLock::reader_exit"};
    }

    // Block until the room is completely empty, then own it.
    void writer_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReadWriteLock::writer_enter"};
    }

    // Give up ownership of the room.
    void writer_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ReadWriteLock::writer_exit"};
    }

  private:
    // TODO: your sync members — something writers hold exclusively, plus
    // whatever the readers need to hold it collectively.
};
