// Koan 13 — Writer-priority readers-writers (starter code). Edit this file only.
//
// Guarantees: readers share; a writer is alone; and once any writer is
// waiting or writing, no NEW reader enters until every queued writer has
// finished. (Readers can starve under a steady writer stream — by design.)
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

class WriterPriorityReadWriteLock {
  public:
    // Block while any writer is inside OR queued; readers share.
    void reader_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"WriterPriorityReadWriteLock::reader_enter"};
    }

    // Leave the room. The last reader out has a special job.
    void reader_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"WriterPriorityReadWriteLock::reader_exit"};
    }

    // Bar new readers the moment you queue; enter once alone.
    void writer_enter() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"WriterPriorityReadWriteLock::writer_enter"};
    }

    // Hand off to the next writer if any; else readmit readers.
    void writer_exit() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"WriterPriorityReadWriteLock::writer_exit"};
    }

  private:
    // TODO: your sync members. Koan 11's collective-claim trick is needed
    // twice here — once per category.
};
