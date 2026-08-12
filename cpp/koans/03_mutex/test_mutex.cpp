#include "koan_test.hpp"
#include "mutex.hpp"

#include <atomic>
#include <memory>
#include <thread>

using namespace koans;

namespace {

// An increment with a deliberately widened read-modify-write window.
// Relaxed atomics keep the race observable without undefined behavior.
class RacyCounter {
  public:
    void increment(int nap_every, int i) {
        int temp = value_.load(std::memory_order_relaxed);
        if (i % nap_every == 0)
            std::this_thread::sleep_for(500us);  // invite an interleaving
        value_.store(temp + 1, std::memory_order_relaxed);
    }

    int value() const { return value_.load(); }

  private:
    std::atomic<int> value_{0};
};

}  // namespace

KOAN_TEST(no_lost_updates) {
    Mutex mutex;
    RacyCounter counter;
    constexpr int kThreads = 4, kIters = 50;
    ThreadRunner runner;
    for (int t = 0; t < kThreads; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < kIters; ++i) {
                mutex.acquire();
                counter.increment(10, i);
                mutex.release();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(counter.value() == kThreads * kIters,
                    "lost updates: expected " +
                        std::to_string(kThreads * kIters) + ", got " +
                        std::to_string(counter.value()) +
                        " — the critical section is not exclusive");
}

KOAN_TEST(critical_section_is_exclusive) {
    Mutex mutex;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 4; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 25; ++i) {
                mutex.acquire();
                tracker.enter("cs");
                jitter(1);
                tracker.exit("cs");
                mutex.release();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("cs") == 1,
                    std::to_string(tracker.max_concurrent("cs")) +
                        " threads were inside the critical section at once");
}

KOAN_TEST(second_acquire_blocks_until_release) {
    auto mutex = std::make_shared<Mutex>();
    assert_completes([mutex] { mutex->acquire(); }, 2000ms, "first acquire");
    auto probe = assert_blocks([mutex] { mutex->acquire(); }, 300ms,
                               "second acquire (mutex is held)");
    mutex->release();
    probe.assert_completed(5000ms, "the blocked acquire after release");
    mutex->release();
}

KOAN_TEST(works_for_many_threads) {
    Mutex mutex;
    RacyCounter counter;
    ThreadRunner runner;
    for (int t = 0; t < 10; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 20; ++i) {
                mutex.acquire();
                counter.increment(7, i);
                mutex.release();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_EQ(counter.value(), 200);
}

// Meta-test: proves the rig can detect a broken mutex. If this ever fails,
// the tests above have lost their teeth; nothing about YOUR code runs here.
KOAN_TEST(sanity_racy_counter_actually_races_without_a_mutex) {
    RacyCounter counter;
    ThreadRunner runner;
    for (int t = 0; t < 4; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 50; ++i) counter.increment(10, i);
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(counter.value() < 200,
                    "the racy counter did not race; scheduling assumptions "
                    "are off");
}
