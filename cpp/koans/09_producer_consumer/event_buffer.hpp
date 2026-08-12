// Koan 09 — Producer-consumer (starter code). Edit this file only.
//
// Guarantees: at most one thread touches the (not thread-safe) buffer at a
// time; consume() blocks while the buffer is empty; produce() never blocks
// indefinitely. The buffer is unbounded in this koan.
#pragma once

#include <mutex>
#include <semaphore>

#include "koans.hpp"

// Buffer has add(int) and int get(). It is NOT thread-safe, and get() on
// an empty buffer is an error.
template <typename Buffer>
class ProducerConsumer {
  public:
    explicit ProducerConsumer(Buffer& buffer) : buffer_(buffer) {}

    // Put `item` into the buffer. Must never block indefinitely.
    void produce(int item) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)item;
        throw koans::NotImplemented{"ProducerConsumer::produce"};
    }

    // Block while the buffer is empty, then remove and return an item.
    int consume() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"ProducerConsumer::consume"};
    }

  private:
    Buffer& buffer_;
    // TODO: your sync members — something granting exclusive access to
    // the buffer, and something counting what's in it.
};
