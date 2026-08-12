#include "koan_test.hpp"
#include "reusable_barrier.hpp"

#include <memory>
#include <thread>

using namespace koans;

namespace {

// Per round r: all n arrivals precede every departure of round r, and
// nobody arrives at round r+1 before everyone has arrived at round r.
void assert_rounds_atomic(const EventLog& log, int n, int rounds) {
    for (int r = 0; r < rounds; ++r) {
        auto arrive = "arrive:" + std::to_string(r);
        auto depart = "depart:" + std::to_string(r);
        KOAN_ASSERT_MSG(log.count(arrive) == static_cast<std::size_t>(n),
                        "round " + std::to_string(r) + ": expected " +
                            std::to_string(n) + " arrivals, saw " +
                            std::to_string(log.count(arrive)));
        KOAN_ASSERT_MSG(log.count(depart) == static_cast<std::size_t>(n),
                        "round " + std::to_string(r) + ": expected " +
                            std::to_string(n) + " departures, saw " +
                            std::to_string(log.count(depart)));
        log.assert_before(arrive, depart);
        if (r + 1 < rounds)
            log.assert_before(arrive, "arrive:" + std::to_string(r + 1));
    }
}

EventLog run_rounds(int n, int rounds, std::chrono::milliseconds join_timeout) {
    auto barrier = std::make_shared<ReusableBarrier>(n);
    auto log = std::make_shared<EventLog>();
    ThreadRunner runner;
    for (int t = 0; t < n; ++t) {
        runner.spawn([barrier, log, rounds] {
            for (int r = 0; r < rounds; ++r) {
                jitter();
                log->record("arrive:" + std::to_string(r));
                barrier->wait();
                log->record("depart:" + std::to_string(r));
                jitter();
            }
        });
    }
    runner.join_all(join_timeout);
    return *log;
}

}  // namespace

KOAN_TEST(rounds_are_atomic) {
    constexpr int n = 4, rounds = 8;
    auto log = run_rounds(n, rounds, 15000ms);
    assert_rounds_atomic(log, n, rounds);
}

KOAN_TEST(nobody_leaks_through_early) {
    constexpr int n = 4;
    auto barrier = std::make_shared<ReusableBarrier>(n);
    auto departed = std::make_shared<EventLog>();
    ThreadRunner runner;
    for (int r = 0; r < 2; ++r) {  // two consecutive rounds, same barrier
        auto tag = "depart:" + std::to_string(r);
        for (int i = 0; i < n - 1; ++i) {
            runner.spawn([barrier, departed, tag] {
                barrier->wait();
                departed->record(tag);
            });
        }
        std::this_thread::sleep_for(250ms);  // plenty of time to misbehave
        KOAN_ASSERT_MSG(departed->count(tag) == 0,
                        "round " + std::to_string(r) + ": " +
                            std::to_string(departed->count(tag)) +
                            " thread(s) got past the barrier with only " +
                            std::to_string(n - 1) + " of " + std::to_string(n) +
                            " arrived");
        runner.spawn([barrier, departed, tag] {  // the nth springs the door
            barrier->wait();
            departed->record(tag);
        });
        runner.join_all(5000ms);
        KOAN_ASSERT_EQ(departed->count(tag), static_cast<std::size_t>(n));
    }
}

// Uses the split-phase API (work between phase1 and phase2 is allowed),
// which is where a resettable-in-name-only barrier lets a fast thread lap
// the field.
KOAN_TEST(many_rounds_stress) {
    constexpr int n = 6, rounds = 40;
    auto barrier = std::make_shared<ReusableBarrier>(n);
    auto log = std::make_shared<EventLog>();
    ThreadRunner runner;
    for (int t = 0; t < n; ++t) {
        runner.spawn([barrier, log] {
            for (int r = 0; r < rounds; ++r) {
                jitter(1);
                log->record("arrive:" + std::to_string(r));
                barrier->phase1();
                jitter(3);  // "work" at the critical point between phases
                barrier->phase2();
                log->record("depart:" + std::to_string(r));
            }
        });
    }
    runner.join_all(15000ms);
    std::size_t total = 0;
    for (int r = 0; r < rounds; ++r)
        total += log->count("depart:" + std::to_string(r));
    KOAN_ASSERT_EQ(total, static_cast<std::size_t>(n * rounds));
    assert_rounds_atomic(*log, n, rounds);
}

KOAN_TEST(works_for_two_batches_of_waits) {
    constexpr int n = 3;
    auto barrier = std::make_shared<ReusableBarrier>(n);
    for (int batch = 0; batch < 2; ++batch) {  // batch, plain join, batch again
        ThreadRunner runner;
        for (int i = 0; i < n; ++i)
            runner.spawn([barrier] { barrier->wait(); });
        runner.join_all(5000ms);
    }
}
