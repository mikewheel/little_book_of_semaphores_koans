#include "koan_test.hpp"
#include "handmade_semaphore.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

KOAN_TEST(initial_value_admits_that_many) {
    auto sem = std::make_shared<HandmadeSemaphore>(2);
    assert_completes([sem] { sem->acquire(); }, 2000ms, "the first acquire");
    assert_completes([sem] { sem->acquire(); }, 2000ms, "the second acquire");
    auto probe = assert_blocks([sem] { sem->acquire(); }, 300ms,
                               "the third acquire (value exhausted)");
    sem->release();
    probe.assert_completed(5000ms, "the third acquire after a release");
}

KOAN_TEST(zero_starts_blocked) {
    auto sem = std::make_shared<HandmadeSemaphore>(0);
    auto probe = assert_blocks([sem] { sem->acquire(); }, 300ms,
                               "acquire on a zero semaphore");
    sem->release();
    probe.assert_completed(5000ms, "the acquire after a release");
}

KOAN_TEST(release_before_acquire_banks_a_token) {
    auto sem = std::make_shared<HandmadeSemaphore>(0);
    sem->release();  // nobody is waiting: the permit must be banked, not lost
    sem->release();
    assert_completes([sem] { sem->acquire(); }, 2000ms,
                     "spending the first banked permit");
    assert_completes([sem] { sem->acquire(); }, 2000ms,
                     "spending the second banked permit");
    auto probe = assert_blocks([sem] { sem->acquire(); }, 300ms,
                               "acquiring from the now-empty bank");
    sem->release();
    probe.assert_completed(5000ms, "the acquire after a release");
}

KOAN_TEST(wakes_exactly_one) {
    auto sem = std::make_shared<HandmadeSemaphore>(0);
    auto done = std::make_shared<std::atomic<int>>(0);
    ThreadRunner runner;
    for (int i = 0; i < 4; ++i)
        runner.spawn(
            [sem, done] {
                sem->acquire();
                done->fetch_add(1);
            },
            "w" + std::to_string(i));
    std::this_thread::sleep_for(300ms);  // let all four park
    KOAN_ASSERT_EQ(done->load(), 0);
    sem->release();
    eventually([done] { return done->load() == 1; }, 5000ms,
               "one waiter should have gotten through");
    std::this_thread::sleep_for(300ms);  // nobody else should sneak through
    KOAN_ASSERT_MSG(done->load() == 1,
                    "one release woke " + std::to_string(done->load()) +
                        " waiters — exactly one must get in");
    for (int i = 0; i < 3; ++i) sem->release();
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(done->load(), 4);
}

// Property 3: a release must go to a parked waiter, and a fresh acquirer
// racing in right behind the release must queue up instead of snatching
// the wakeup (the classic stolen-signal bug).
KOAN_TEST(released_token_is_reserved_for_a_waiter) {
    auto sem = std::make_shared<HandmadeSemaphore>(0);
    ThreadRunner runner;
    for (int round = 0; round < 6; ++round) {
        auto got_it = std::make_shared<std::atomic<bool>>(false);
        runner.spawn(
            [sem, got_it] {
                sem->acquire();
                got_it->store(true);
            },
            "w" + std::to_string(round));
        std::this_thread::sleep_for(300ms);  // let the waiter park
        // Release, then immediately re-acquire from the same thread: the
        // thief-shaped move. The permit is spoken for, so this must block.
        auto probe = assert_blocks(
            [sem] {
                sem->release();
                sem->acquire();
            },
            300ms,
            "a release immediately followed by acquire (round " +
                std::to_string(round) + ") — it stole the wakeup reserved "
                "for the parked waiter");
        eventually([got_it] { return got_it->load(); }, 5000ms,
                   "the parked waiter never received the released permit "
                   "(round " + std::to_string(round) + ")");
        sem->release();  // now free the would-be thief
        probe.assert_completed(5000ms, "the queued-up acquire");
    }
    runner.join_all(10000ms);
}

KOAN_TEST(no_lost_wakeups_stress) {
    auto ping = std::make_shared<HandmadeSemaphore>(0);
    auto pong = std::make_shared<HandmadeSemaphore>(0);
    auto finished = std::make_shared<std::atomic<int>>(0);
    constexpr int rounds = 50;
    ThreadRunner runner;
    for (int i = 0; i < 4; ++i) {
        runner.spawn(
            [ping, pong, finished] {
                for (int r = 0; r < rounds; ++r) {
                    ping->release();
                    pong->acquire();
                    jitter(1);
                }
                finished->fetch_add(1);
            },
            "ping" + std::to_string(i));
        runner.spawn(
            [ping, pong, finished] {
                for (int r = 0; r < rounds; ++r) {
                    ping->acquire();
                    pong->release();
                    jitter(1);
                }
                finished->fetch_add(1);
            },
            "pong" + std::to_string(i));
    }
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(finished->load(), 8);
}

KOAN_TEST(works_as_mutex) {
    auto sem = std::make_shared<HandmadeSemaphore>(1);
    constexpr int per_thread = 2000;
    // Deliberately a plain int (not atomic): only the semaphore guards it.
    // shared_ptr-owned so a stuck worker can never dangle (see header note).
    auto counter = std::make_shared<int>(0);
    ThreadRunner runner;
    for (int i = 0; i < 8; ++i)
        runner.spawn(
            [sem, counter] {
                for (int n = 0; n < per_thread; ++n) {
                    sem->acquire();
                    ++*counter;
                    sem->release();
                }
            },
            "m" + std::to_string(i));
    runner.join_all(10000ms);
    KOAN_ASSERT_MSG(*counter == 8 * per_thread,
                    "lost updates: got " + std::to_string(*counter) +
                        ", expected " + std::to_string(8 * per_thread) +
                        " — the semaphore does not provide mutual exclusion");
}
