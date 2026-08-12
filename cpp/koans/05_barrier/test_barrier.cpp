#include "koan_test.hpp"
#include "barrier.hpp"

#include <thread>

using namespace koans;

KOAN_TEST(nobody_passes_before_the_last_arrival) {
    constexpr int n = 5;
    Barrier barrier(n);
    EventLog passed;
    ThreadRunner runner;
    for (int i = 0; i < n - 1; ++i) {
        runner.spawn([&, i] {
            barrier.wait();
            passed.record("t" + std::to_string(i));
        });
    }
    // Give the early arrivals every chance to leak through.
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(passed.events().empty(),
                    std::to_string(passed.events().size()) +
                        " thread(s) passed the barrier before all " +
                        std::to_string(n) + " arrived");
    runner.spawn([&] {  // the nth arrival springs the door
        barrier.wait();
        passed.record("t_last");
    });
    runner.join_all(5000ms);
    KOAN_ASSERT_MSG(passed.events().size() == n,
                    "only " + std::to_string(passed.events().size()) + " of " +
                        std::to_string(n) +
                        " threads passed — did the nth arrival wake exactly "
                        "one sleeper?");
}

KOAN_TEST(all_threads_pass_when_arrivals_are_staggered) {
    constexpr int n = 4;
    Barrier barrier(n);
    EventLog passed;
    ThreadRunner runner;
    for (int i = 0; i < n; ++i) {
        runner.spawn([&, i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * i));
            barrier.wait();
            passed.record("t" + std::to_string(i));
        });
    }
    runner.join_all(5000ms);
    KOAN_ASSERT_EQ(passed.events().size(), static_cast<std::size_t>(n));
}

KOAN_TEST(late_straggler_releases_everyone) {
    constexpr int n = 3;
    Barrier barrier(n);
    EventLog passed;
    ThreadRunner runner;
    for (int i = 0; i < 2; ++i) {
        runner.spawn([&, i] {
            barrier.wait();
            passed.record("t" + std::to_string(i));
        });
    }
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT(passed.events().empty());
    runner.spawn([&] {
        barrier.wait();
        passed.record("t2");
    });
    eventually([&] { return passed.events().size() == n; }, 5000ms,
               "the last arrival did not release all waiting threads");
    runner.join_all(5000ms);
}

KOAN_TEST(stress_various_sizes) {
    for (int n : {2, 7, 16}) {
        Barrier barrier(n);
        EventLog passed;
        ThreadRunner runner;
        for (int i = 0; i < n; ++i) {
            runner.spawn([&, i] {
                jitter();
                barrier.wait();
                passed.record("t" + std::to_string(i));
            });
        }
        runner.join_all(10000ms);
        KOAN_ASSERT_MSG(passed.events().size() == static_cast<std::size_t>(n),
                        "n=" + std::to_string(n) + ": not everyone passed");
    }
}
