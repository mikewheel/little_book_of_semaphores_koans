// Koan 14 — No-starve mutex (starter code). Edit this file only.
//
// Guarantee: acquire()/release() give mutual exclusion, and once a thread
// has called acquire(), the number of times OTHER threads can be granted
// the lock before it gets in is bounded — even though the only building
// block is a semaphore that wakes waiters at random.
//
// Honor rule: NoStarveMutex may use only WeakSemaphore members and plain
// integers. No std::mutex, std::semaphore, or std::condition_variable of
// your own.
#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <vector>

#include "koans.hpp"

// A semaphore with only the *weak* guarantee (provided — do not modify).
//
// Two deliberately adversarial behaviors, both legal for a semaphore that
// promises no more than "a signal wakes someone":
//  - release() wakes a RANDOM waiter — never assume first-come-first-served.
//  - a release() that finds no waiters banks a token that any LATER arrival
//    may snatch, even if an earlier thread was already mid-approach.
class WeakSemaphore {
  public:
    explicit WeakSemaphore(int value = 0) : value_(value) {}

    void acquire() {
        std::unique_lock<std::mutex> lock(lock_);
        if (value_ > 0) {
            --value_;
            return;
        }
        auto me = std::make_shared<Waiter>();
        waiters_.push_back(me);
        me->cv.wait(lock, [&] { return me->granted; });
    }

    void release() {
        std::lock_guard<std::mutex> lock(lock_);
        if (!waiters_.empty()) {
            std::uniform_int_distribution<std::size_t> pick(
                0, waiters_.size() - 1);
            std::size_t i = pick(rng_);
            auto chosen = waiters_[i];
            waiters_.erase(waiters_.begin() + static_cast<std::ptrdiff_t>(i));
            chosen->granted = true;  // token handed directly to the waiter
            chosen->cv.notify_one();
        } else {
            ++value_;
        }
    }

  private:
    struct Waiter {
        std::condition_variable cv;
        bool granted = false;
    };
    std::mutex lock_;
    int value_;
    std::vector<std::shared_ptr<Waiter>> waiters_;
    std::mt19937 rng_{std::random_device{}()};
};

// A mutex with bounded overtaking, built from weak semaphores alone.
class NoStarveMutex {
  public:
    // Block until the lock is yours. Overtaking must stay bounded.
    void acquire() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveMutex::acquire"};
    }

    // Hand the lock on. Pair with acquire().
    void release() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"NoStarveMutex::release"};
    }

  private:
    // TODO: your sync members — WeakSemaphore instances and plain ints only.
};
