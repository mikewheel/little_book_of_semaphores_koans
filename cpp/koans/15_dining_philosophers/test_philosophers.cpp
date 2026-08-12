#include "koan_test.hpp"
#include "philosophers.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using namespace koans;

namespace {

constexpr int kSeats = 5;

std::pair<int, int> neighbors(int i) {
    return {(i + kSeats - 1) % kSeats, (i + 1) % kSeats};
}

std::string eat_label(int i) { return "eat:" + std::to_string(i); }

struct Dinner {
    std::shared_ptr<Table> table = std::make_shared<Table>(kSeats);
    std::shared_ptr<OverlapTracker> tracker = std::make_shared<OverlapTracker>();
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();
    ThreadRunner runner;

    // Every philosopher captures shared_ptr state by value: if a wrong
    // solution wedges and join_all gives up, the parked threads never
    // touch freed memory.
    void run(int meals, int think_ms, int eat_ms,
             std::chrono::milliseconds join_timeout) {
        for (int i = 0; i < kSeats; ++i) {
            auto table_ = table;
            auto tracker_ = tracker;
            auto log_ = log;
            runner.spawn(
                [table_, tracker_, log_, i, meals, think_ms, eat_ms] {
                    auto [prev, next] = neighbors(i);
                    for (int m = 0; m < meals; ++m) {
                        if (think_ms) jitter(think_ms);
                        table_->get_forks(i);
                        auto snapshot = tracker_->enter(eat_label(i));
                        if (snapshot[eat_label(prev)] > 0 ||
                            snapshot[eat_label(next)] > 0)
                            tracker_->violate(
                                "philosopher " + std::to_string(i) +
                                " ate at the same time as a neighbor");
                        if (eat_ms) jitter(eat_ms);
                        tracker_->exit(eat_label(i));
                        table_->put_forks(i);
                        log_->record("meal:" + std::to_string(i));
                    }
                },
                "phil-" + std::to_string(i));
        }
        runner.join_all(join_timeout);
    }
};

}  // namespace

KOAN_TEST(neighbors_never_eat_together) {
    Dinner dinner;
    dinner.run(20, 2, 1, 15000ms);
    dinner.tracker->assert_no_violations();
}

// Philosophers 0 and 1 share a fork: while 0 eats, 1 must wait.
KOAN_TEST(hungry_neighbor_blocks_until_forks_return) {
    auto table = std::make_shared<Table>(kSeats);
    assert_completes([table] { table->get_forks(0); }, 2000ms,
                     "get_forks at an empty table");
    auto probe = assert_blocks([table] { table->get_forks(1); }, 300ms,
                               "philosopher 1 (neighbor 0 holds their fork)");
    table->put_forks(0);
    probe.assert_completed(5000ms, "philosopher 1 once the forks return");
    table->put_forks(1);
}

// Seats 0 and 2 share no fork; the table must let them eat at once.
KOAN_TEST(two_nonadjacent_philosophers_eat_together) {
    auto table = std::make_shared<Table>(kSeats);
    auto tracker = std::make_shared<OverlapTracker>();
    auto done = std::make_shared<std::atomic<bool>>(false);
    ThreadRunner runner;
    for (int i : {0, 2}) {
        runner.spawn(
            [table, tracker, done, i] {
                table->get_forks(i);
                tracker->enter(eat_label(i));
                auto deadline = std::chrono::steady_clock::now() + 5s;
                while (!done->load() &&
                       std::chrono::steady_clock::now() < deadline)
                    std::this_thread::sleep_for(1ms);  // linger, observably
                tracker->exit(eat_label(i));
                table->put_forks(i);
            },
            "phil-" + std::to_string(i));
    }
    eventually(
        [&] {
            return tracker->current("eat:0") == 1 &&
                   tracker->current("eat:2") == 1;
        },
        5000ms,
        "philosophers 0 and 2 share no fork but never ate together — the "
        "table over-serializes");
    done->store(true);
    runner.join_all(5000ms);
}

// Zero think time, everyone grabbing at once: the classic wedge. A
// symmetric grab-your-right-fork-first solution deadlocks here within a
// few rounds; the join deadline turns that into a failure, not a hang.
KOAN_TEST(no_deadlock_when_everyone_is_hungry) {
    constexpr int meals = 1000;
    Dinner dinner;
    dinner.run(meals, 0, 0, 15000ms);
    dinner.tracker->assert_no_violations();
    for (int i = 0; i < kSeats; ++i)
        KOAN_ASSERT_EQ(dinner.log->count("meal:" + std::to_string(i)),
                       static_cast<std::size_t>(meals));
}

KOAN_TEST(everyone_eats_and_meals_overlap) {
    constexpr int meals = 20;
    Dinner dinner;
    dinner.run(meals, 1, 1, 15000ms);
    dinner.tracker->assert_no_violations();
    for (int i = 0; i < kSeats; ++i) {
        auto got = dinner.log->count("meal:" + std::to_string(i));
        KOAN_ASSERT_MSG(got == meals,
                        "philosopher " + std::to_string(i) +
                            " finished only " + std::to_string(got) + " of " +
                            std::to_string(meals) + " meals");
    }
    KOAN_ASSERT_MSG(dinner.tracker->max_combined() >= 2,
                    "no two philosophers ever ate at the same time — the "
                    "table over-serializes");
}
