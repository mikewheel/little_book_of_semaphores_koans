// koan_test.hpp — a tiny, dependency-free test harness for the semaphore
// koans. One test executable per koan; this header supplies main().
//
// Design notes (worth a read — this is idiomatic "testing concurrent code"
// machinery you may want to reuse someday):
//
//  * Every test runs on a watchdog: the test body executes in its own
//    thread while main() waits with a deadline. C++ has no portable way to
//    kill a stuck thread, so on timeout we report a suspected deadlock and
//    _Exit the process — ctest reports the failure and the remaining tests
//    are skipped until you fix the hang.
//
//  * Helpers like assert_blocks() intentionally leave a thread parked on
//    your semaphores. Such probes capture shared_ptr state so the leaked
//    thread never dangles. Tests written here follow that rule; do the same
//    if you extend them.
//
//  * Tests provoke races with jitter() (random microsleeps) and repetition.
//    Passing is strong evidence of correctness, not a proof — the honest
//    limit of all concurrency testing.

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "koans.hpp"

namespace koans {

using namespace std::chrono_literals;

// ── Failure types ────────────────────────────────────────────────────────

struct AssertionFailure : std::runtime_error {
    AssertionFailure(const char* file, int line, const std::string& message)
        : std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                             ": " + message) {}
};

#define KOAN_FAIL(msg)                                                        \
    throw ::koans::AssertionFailure(__FILE__, __LINE__, (msg))

#define KOAN_ASSERT(cond)                                                     \
    do {                                                                      \
        if (!(cond)) KOAN_FAIL("assertion failed: " #cond);                   \
    } while (0)

#define KOAN_ASSERT_MSG(cond, msg)                                            \
    do {                                                                      \
        if (!(cond))                                                          \
            KOAN_FAIL(std::string("assertion failed: " #cond " — ") + (msg)); \
    } while (0)

#define KOAN_ASSERT_EQ(a, b)                                                  \
    do {                                                                      \
        auto _ka = (a);                                                       \
        auto _kb = (b);                                                       \
        if (!(_ka == _kb)) {                                                  \
            std::ostringstream _os;                                           \
            _os << "expected " #a " == " #b " but got " << _ka                \
                << " != " << _kb;                                             \
            KOAN_FAIL(_os.str());                                             \
        }                                                                     \
    } while (0)

// ── Test registry ────────────────────────────────────────────────────────

struct TestCase {
    std::string name;
    std::function<void()> fn;
    std::chrono::milliseconds timeout;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn,
              std::chrono::milliseconds timeout) {
        registry().push_back({std::move(name), std::move(fn), timeout});
    }
};

#define KOAN_TEST_WITH_TIMEOUT(name, timeout_ms)                              \
    static void koan_test_##name();                                           \
    static ::koans::Registrar koan_registrar_##name{                          \
        #name, &koan_test_##name, std::chrono::milliseconds(timeout_ms)};     \
    static void koan_test_##name()

#define KOAN_TEST(name) KOAN_TEST_WITH_TIMEOUT(name, 20000)

// ── Concurrency test utilities ───────────────────────────────────────────

// Sleep a random sliver of time to provoke unlucky schedules.
inline void jitter(int max_ms = 2) {
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, max_ms * 1000);
    std::this_thread::sleep_for(std::chrono::microseconds(dist(rng)));
}

// A thread-safe, ordered record of named events.
class EventLog {
  public:
    EventLog() = default;
    // Copyable so helpers can return a finished log by value (the mutex and
    // condition variable themselves are not copied).
    EventLog(const EventLog& other) : events_(other.events()) {}
    EventLog& operator=(const EventLog& other) {
        if (this != &other) {
            auto evs = other.events();
            std::lock_guard lock(mutex_);
            events_ = std::move(evs);
        }
        return *this;
    }

    void record(const std::string& label) {
        std::lock_guard lock(mutex_);
        events_.push_back(label);
        cond_.notify_all();
    }

    std::vector<std::string> events() const {
        std::lock_guard lock(mutex_);
        return events_;
    }

    std::size_t count(const std::string& label) const {
        std::lock_guard lock(mutex_);
        return static_cast<std::size_t>(
            std::count(events_.begin(), events_.end(), label));
    }

    // Assert every occurrence of `first` precedes every occurrence of `second`.
    void assert_before(const std::string& first, const std::string& second) const {
        auto evs = events();
        long last_first = -1, first_second = -1;
        for (std::size_t i = 0; i < evs.size(); ++i) {
            if (evs[i] == first) last_first = static_cast<long>(i);
            if (evs[i] == second && first_second < 0)
                first_second = static_cast<long>(i);
        }
        if (last_first < 0) KOAN_FAIL("no '" + first + "' event was recorded");
        if (first_second < 0) KOAN_FAIL("no '" + second + "' event was recorded");
        if (last_first > first_second)
            KOAN_FAIL("expected all '" + first + "' events before any '" +
                      second + "' event; log was " + joined());
    }

    void wait_for_count(const std::string& label, std::size_t n,
                        std::chrono::milliseconds timeout = 10000ms) const {
        std::unique_lock lock(mutex_);
        bool ok = cond_.wait_for(lock, timeout, [&] {
            return static_cast<std::size_t>(std::count(events_.begin(),
                                                       events_.end(), label)) >= n;
        });
        if (!ok)
            KOAN_FAIL("timed out waiting for " + std::to_string(n) + " x '" +
                      label + "' events");
    }

    std::string joined() const {
        auto evs = events();
        std::string out = "[";
        for (std::size_t i = 0; i < evs.size(); ++i) {
            out += evs[i];
            if (i + 1 < evs.size()) out += ", ";
        }
        return out + "]";
    }

  private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cond_;
    std::vector<std::string> events_;
};

// Tracks how many threads are inside named sections at the same time.
// enter() returns a snapshot of the occupancy *including the caller*, so
// tests can check invariants at the moment of entry.
class OverlapTracker {
  public:
    std::map<std::string, int> enter(const std::string& category) {
        std::lock_guard lock(mutex_);
        int now = ++current_[category];
        max_[category] = std::max(max_[category], now);
        int combined = 0;
        for (auto& [_, v] : current_) combined += v;
        max_combined_ = std::max(max_combined_, combined);
        return current_;
    }

    void exit(const std::string& category) {
        std::lock_guard lock(mutex_);
        --current_[category];
    }

    void violate(const std::string& message) {
        std::lock_guard lock(mutex_);
        violations_.push_back(message);
    }

    void assert_no_violations() const {
        std::lock_guard lock(mutex_);
        if (!violations_.empty())
            KOAN_FAIL(std::to_string(violations_.size()) +
                      " invariant violation(s); first: " + violations_.front());
    }

    int max_concurrent(const std::string& category) const {
        std::lock_guard lock(mutex_);
        auto it = max_.find(category);
        return it == max_.end() ? 0 : it->second;
    }

    int max_combined() const {
        std::lock_guard lock(mutex_);
        return max_combined_;
    }

    int current(const std::string& category) const {
        std::lock_guard lock(mutex_);
        auto it = current_.find(category);
        return it == current_.end() ? 0 : it->second;
    }

  private:
    mutable std::mutex mutex_;
    std::map<std::string, int> current_;
    std::map<std::string, int> max_;
    int max_combined_ = 0;
    std::vector<std::string> violations_;
};

// Spawns worker threads, collects their exceptions, joins with a deadline.
class ThreadRunner {
  public:
    template <typename F>
    void spawn(F&& fn, std::string name = "worker") {
        auto state = state_;
        state->pending.fetch_add(1);
        threads_.emplace_back(
            [state, name = std::move(name), fn = std::forward<F>(fn)]() mutable {
                try {
                    fn();
                } catch (...) {
                    std::lock_guard lock(state->mutex);
                    if (state->error_message.empty()) {
                        try {
                            throw;
                        } catch (const std::exception& e) {
                            state->error_message =
                                "worker '" + name + "' raised: " + e.what();
                        } catch (...) {
                            state->error_message =
                                "worker '" + name + "' raised a non-exception";
                        }
                    }
                }
                state->pending.fetch_sub(1);
            });
    }

    // Join every spawned thread; fail fast on worker errors, and fail with a
    // deadlock diagnosis if workers are still running at the deadline.
    void join_all(std::chrono::milliseconds timeout = 10000ms) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            rethrow_worker_error();
            if (state_->pending.load() == 0) {
                for (auto& t : threads_) t.join();
                threads_.clear();
                rethrow_worker_error();
                return;
            }
            std::this_thread::sleep_for(5ms);
        }
        rethrow_worker_error();
        int stuck = state_->pending.load();
        for (auto& t : threads_) t.detach();  // watchdog will handle the rest
        threads_.clear();
        KOAN_FAIL(std::to_string(stuck) +
                  " worker thread(s) never finished "
                  "(deadlock, or a missing signal?)");
    }

    ~ThreadRunner() {
        for (auto& t : threads_)
            if (t.joinable()) t.detach();
    }

  private:
    struct State {
        std::atomic<int> pending{0};
        std::mutex mutex;
        std::string error_message;
    };

    void rethrow_worker_error() {
        std::lock_guard lock(state_->mutex);
        if (!state_->error_message.empty())
            throw AssertionFailure("worker", 0, state_->error_message);
    }

    std::shared_ptr<State> state_ = std::make_shared<State>();
    std::vector<std::thread> threads_;
};

// Handle to a call being probed by assert_blocks/assert_completes.
struct Probe {
    std::shared_ptr<std::atomic<bool>> done =
        std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
    std::shared_ptr<std::condition_variable> cond =
        std::make_shared<std::condition_variable>();
    std::shared_ptr<std::string> error = std::make_shared<std::string>();

    bool wait(std::chrono::milliseconds timeout) const {
        std::unique_lock lock(*mutex);
        return cond->wait_for(lock, timeout, [&] { return done->load(); });
    }

    void assert_completed(std::chrono::milliseconds timeout,
                          const std::string& what) const {
        if (!wait(timeout))
            KOAN_FAIL(what + " should have completed by now "
                      "(deadlock, or a missing signal?)");
        if (!error->empty()) KOAN_FAIL(what + " raised: " + *error);
    }
};

// Assert that fn() does NOT return (or throw) within `timeout`; returns a
// Probe the test can use later to assert it eventually completed. fn is
// copied into heap state so the parked thread can never dangle — but any
// objects fn references must themselves outlive the block (use shared_ptr).
template <typename F>
Probe assert_blocks(F fn, std::chrono::milliseconds timeout = 300ms,
                    const std::string& what = "the call") {
    Probe probe;
    std::thread([probe, fn = std::move(fn)]() mutable {
        try {
            fn();
        } catch (const std::exception& e) {
            *probe.error = e.what();
        } catch (...) {
            *probe.error = "non-exception thrown";
        }
        {
            std::lock_guard lock(*probe.mutex);
            probe.done->store(true);
        }
        probe.cond->notify_all();
    }).detach();
    if (probe.wait(timeout)) {
        if (!probe.error->empty())
            KOAN_FAIL("expected " + what + " to block, but it raised: " +
                      *probe.error);
        KOAN_FAIL("expected " + what + " to block, but it returned");
    }
    return probe;
}

// Assert that fn() completes within `timeout`.
template <typename F>
void assert_completes(F fn, std::chrono::milliseconds timeout = 10000ms,
                      const std::string& what = "the call") {
    Probe probe;
    std::thread([probe, fn = std::move(fn)]() mutable {
        try {
            fn();
        } catch (const std::exception& e) {
            *probe.error = e.what();
        } catch (...) {
            *probe.error = "non-exception thrown";
        }
        {
            std::lock_guard lock(*probe.mutex);
            probe.done->store(true);
        }
        probe.cond->notify_all();
    }).detach();
    probe.assert_completed(timeout, what);
}

// Poll until predicate() is true; fail if the timeout expires.
template <typename P>
void eventually(P predicate, std::chrono::milliseconds timeout = 10000ms,
                const std::string& msg = "condition never became true") {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return;
        std::this_thread::sleep_for(5ms);
    }
    KOAN_FAIL(msg);
}

// ── Runner ───────────────────────────────────────────────────────────────

inline int run_all_tests() {
    int failures = 0;
    for (auto& test : registry()) {
        std::printf("  • %-50s ", test.name.c_str());
        std::fflush(stdout);

        auto state = std::make_shared<Probe>();
        std::string error_kind;
        auto errmsg = std::make_shared<std::string>();
        std::thread([state, errmsg, &test] {
            try {
                test.fn();
            } catch (const NotImplemented& e) {
                *errmsg = std::string("🧘 ") + e.what() +
                          " — open the starter file and fill in the TODOs";
            } catch (const std::exception& e) {
                *errmsg = e.what();
            } catch (...) {
                *errmsg = "unknown exception";
            }
            {
                std::lock_guard lock(*state->mutex);
                state->done->store(true);
            }
            state->cond->notify_all();
        }).detach();

        if (!state->wait(test.timeout)) {
            std::printf("⏱ TIMEOUT after %llds — almost certainly a deadlock.\n",
                        static_cast<long long>(test.timeout.count() / 1000));
            std::printf(
                "\nA test hung, so the remaining tests in this koan were "
                "skipped.\nLook for a thread waiting on a semaphore that "
                "nobody will ever signal.\n");
            std::fflush(stdout);
            std::_Exit(3);
        }
        if (errmsg->empty()) {
            std::printf("✓\n");
        } else {
            ++failures;
            std::printf("✗\n      %s\n", errmsg->c_str());
        }
    }
    if (failures == 0) {
        std::printf("  all %zu tests passed 🎉\n", registry().size());
    } else {
        std::printf("  %d of %zu tests failing\n", failures, registry().size());
    }
    return failures == 0 ? 0 : 1;
}

}  // namespace koans

#ifndef KOAN_NO_MAIN
int main() { return ::koans::run_all_tests(); }
#endif
