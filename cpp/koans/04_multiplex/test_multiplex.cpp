#include "koan_test.hpp"
#include "multiplex.hpp"

#include <condition_variable>
#include <memory>
#include <thread>

using namespace koans;

KOAN_TEST(never_more_than_n_inside) {
    constexpr int n = 4;
    Multiplex plex(n);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 12; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 20; ++i) {
                plex.enter();
                tracker.enter("room");
                jitter(1);
                tracker.exit("room");
                plex.exit();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("room") <= n,
                    "room capacity " + std::to_string(n) + " exceeded: saw " +
                        std::to_string(tracker.max_concurrent("room")));
}

// A mutex in a trench coat must not pass: full concurrency is required.
KOAN_TEST(n_threads_can_be_inside_together) {
    constexpr int n = 4;
    Multiplex plex(n);
    OverlapTracker tracker;
    std::atomic<bool> all_in{false};
    ThreadRunner runner;
    for (int t = 0; t < n; ++t) {
        runner.spawn([&] {
            plex.enter();
            tracker.enter("room");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!all_in.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);  // linger until all inside
            tracker.exit("room");
            plex.exit();
        });
    }
    eventually([&] { return tracker.current("room") == n; }, 5000ms,
               "not all " + std::to_string(n) +
                   " threads made it inside concurrently — the multiplex "
                   "over-serializes");
    all_in.store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(thread_n_plus_1_blocks_then_proceeds_after_an_exit) {
    constexpr int n = 3;
    auto plex = std::make_shared<Multiplex>(n);
    for (int i = 0; i < n; ++i)
        assert_completes([plex] { plex->enter(); }, 2000ms, "an in-capacity entry");
    auto probe = assert_blocks([plex] { plex->enter(); }, 300ms,
                               "entry " + std::to_string(n + 1) +
                                   " (room is full)");
    plex->exit();
    probe.assert_completed(5000ms, "the blocked entry after an exit");
    for (int i = 0; i < n; ++i) plex->exit();
}

KOAN_TEST(capacity_one_degenerates_to_a_mutex) {
    auto plex = std::make_shared<Multiplex>(1);
    assert_completes([plex] { plex->enter(); }, 2000ms, "first entry");
    auto probe = assert_blocks([plex] { plex->enter(); });
    plex->exit();
    probe.assert_completed(5000ms, "the second entry after an exit");
    plex->exit();
}

KOAN_TEST(stress_various_sizes) {
    for (int n : {2, 5, 8}) {
        Multiplex plex(n);
        OverlapTracker tracker;
        ThreadRunner runner;
        for (int t = 0; t < n * 3; ++t) {
            runner.spawn([&] {
                for (int i = 0; i < 10; ++i) {
                    plex.enter();
                    tracker.enter("room");
                    jitter(1);
                    tracker.exit("room");
                    plex.exit();
                }
            });
        }
        runner.join_all(30000ms);
        KOAN_ASSERT(tracker.max_concurrent("room") <= n);
    }
}
