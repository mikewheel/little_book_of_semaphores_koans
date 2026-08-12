// Koan 10 — Bounded buffer (starter code). Edit this file only.
//
// Guarantees: at most one thread touches the (not thread-safe) buffer at a
// time; consume() blocks while the buffer is empty; produce() blocks while
// the buffer already holds `capacity` items, so it never overfills.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

// Buffer has add(int) and int get(). It is NOT thread-safe, and get() on
// an empty buffer is an error.
template <typename Buffer>
class BoundedBuffer {
  public:
    BoundedBuffer(Buffer& buffer, int capacity)
        : buffer_(buffer), capacity_(capacity) {
        // TODO: initialize your sync members. Koan 09's roster, plus one
        // more to account for room that hasn't been used yet.
        (void)capacity_;
    }

    // Block while the buffer is full, then put `item` into it.
    void produce(int item) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)item;
        throw koans::NotImplemented{"BoundedBuffer::produce"};
    }

    // Block while the buffer is empty, then remove and return an item.
    int consume() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"BoundedBuffer::consume"};
    }

  private:
    Buffer& buffer_;
    int capacity_;
    // TODO: your sync members.
};
