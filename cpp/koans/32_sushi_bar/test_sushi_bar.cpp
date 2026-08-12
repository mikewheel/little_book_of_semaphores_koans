#include "koan_test.hpp"
#include "sushi_bar.hpp"

#include <memory>
#include <string>
#include <thread>

using namespace koans;

KOAN_TEST(seats_capacity) {
    SushiBar bar(5);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 10; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 10; ++i) {
                jitter();
                bar.dine([&] {
                    tracker.enter("seat");
                    jitter(1);
                    tracker.exit("seat");
                });
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("seat") <= 5,
                    "more diners than seats: " +
                        std::to_string(tracker.max_concurrent("seat")));
}

KOAN_TEST(no_wait_when_partly_full) {
    auto bar = std::make_shared<SushiBar>();
    auto log = std::make_shared<EventLog>();
    auto hold = std::make_shared<std::counting_semaphore<>>(0);

    // Three incumbents sit and linger; the first through assert_blocks so
    // a broken solution fails fast.
    auto incumbent = [bar, log, hold] {
        bar->dine([log, hold] {
            log->record("incumbent_seated");
            (void)hold->try_acquire_for(10s);
        });
    };
    auto inc_probe = assert_blocks(incumbent, 300ms,
                                   "an incumbent diner (lingering)");
    for (int i = 0; i < 2; ++i) std::thread(incumbent).detach();
    log->wait_for_count("incumbent_seated", 3, 5000ms);

    // 3 of 5 seats taken, no must-wait: the 4th customer sits immediately.
    assert_completes(
        [bar, log] { bar->dine([log] { log->record("fourth_seated"); }); },
        2000ms,
        "an immediate seat while seats are free and the bar never filled");
    hold->release(3);
    inc_probe.assert_completed(5000ms, "the incumbents' meals");
}

namespace {

void cohort_trial() {
    auto bar = std::make_shared<SushiBar>();
    auto log = std::make_shared<EventLog>();
    auto tracker = std::make_shared<OverlapTracker>();
    auto first_two = std::make_shared<std::counting_semaphore<>>(0);
    auto last_three = std::make_shared<std::counting_semaphore<>>(0);

    // Five incumbents fill the bar; the party is now closed.
    auto incumbent = [bar, log, tracker](
                         std::shared_ptr<std::counting_semaphore<>> gate) {
        bar->dine([log, tracker, gate] {
            tracker->enter("seat");
            log->record("incumbent_seated");
            (void)gate->try_acquire_for(10s);
            tracker->exit("seat");
        });
    };
    auto inc_probe = assert_blocks([incumbent, first_two] { incumbent(first_two); },
                                   300ms, "an incumbent diner (lingering)");
    std::thread([incumbent, first_two] { incumbent(first_two); }).detach();
    for (int i = 0; i < 3; ++i)
        std::thread([incumbent, last_three] { incumbent(last_three); }).detach();
    log->wait_for_count("incumbent_seated", 5, 5000ms);

    // Two customers arrive at the full bar: they must wait.
    for (int i = 0; i < 2; ++i) {
        std::thread([bar, log, tracker] {
            log->record("waiter_arrived");
            bar->dine([log, tracker] {
                tracker->enter("seat");
                log->record("waiter_seated");
                std::this_thread::sleep_for(10ms);
                tracker->exit("seat");
            });
        }).detach();
    }
    log->wait_for_count("waiter_arrived", 2, 5000ms);
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("waiter_seated") == 0,
                    "customers sat down at a full bar");

    // Two incumbents leave. Seats are free — but the bar hasn't emptied,
    // so the waiters must STILL be waiting.
    first_two->release(2);
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("waiter_seated") == 0,
                    "a waiter took a freed seat before the bar emptied — "
                    "must-wait mode ended too early");

    // The last three leave; the whole waiting cohort sits together. A wave
    // of fresh arrivals lands at the same moment and must not overfill
    // the bar.
    last_three->release(3);
    for (int i = 0; i < 6; ++i) {
        std::thread([bar, log, tracker] {
            bar->dine([log, tracker] {
                tracker->enter("seat");
                log->record("newcomer_seated");
                std::this_thread::sleep_for(10ms);
                tracker->exit("seat");
            });
        }).detach();
    }
    log->wait_for_count("waiter_seated", 2, 5000ms);
    log->wait_for_count("newcomer_seated", 6, 5000ms);
    inc_probe.assert_completed(5000ms, "the incumbents' meals");
    KOAN_ASSERT_MSG(tracker->max_concurrent("seat") <= 5,
                    "the bar overfilled during the reseat: " +
                        std::to_string(tracker->max_concurrent("seat")) +
                        " diners at once");
}

}  // namespace

KOAN_TEST(full_bar_forces_cohort_wait) {
    for (int i = 0; i < 5; ++i) cohort_trial();
}

KOAN_TEST(mode_resets) {
    auto bar = std::make_shared<SushiBar>();
    auto log = std::make_shared<EventLog>();
    auto incumbents_hold = std::make_shared<std::counting_semaphore<>>(0);
    auto cohort_hold = std::make_shared<std::counting_semaphore<>>(0);

    auto incumbent = [bar, log, incumbents_hold] {
        bar->dine([log, incumbents_hold] {
            log->record("incumbent_seated");
            (void)incumbents_hold->try_acquire_for(10s);
        });
    };
    auto inc_probe = assert_blocks(incumbent, 300ms,
                                   "an incumbent diner (lingering)");
    for (int i = 0; i < 4; ++i) std::thread(incumbent).detach();
    log->wait_for_count("incumbent_seated", 5, 5000ms);

    // Two arrive at the full bar and wait out the party.
    for (int i = 0; i < 2; ++i) {
        std::thread([bar, log, cohort_hold] {
            bar->dine([log, cohort_hold] {
                log->record("waiter_seated");
                (void)cohort_hold->try_acquire_for(10s);
            });
        }).detach();
    }
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT(log->count("waiter_seated") == 0);

    // The bar empties; the 2-person cohort seats; must-wait mode is over.
    incumbents_hold->release(5);
    log->wait_for_count("waiter_seated", 2, 5000ms);

    // 2 of 5 seats taken and the mode reset: a newcomer sits immediately.
    assert_completes(
        [bar, log] { bar->dine([log] { log->record("newcomer_seated"); }); },
        2000ms,
        "an immediate seat after the bar emptied and the cohort sat down "
        "(must-wait mode failed to reset)");
    cohort_hold->release(2);
    inc_probe.assert_completed(5000ms, "the incumbents' meals");
}

KOAN_TEST(stress_capacity_and_totals) {
    SushiBar bar;
    OverlapTracker tracker;
    EventLog log;
    ThreadRunner runner;
    for (int t = 0; t < 25; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 8; ++i) {
                jitter(2);
                bar.dine([&] {
                    tracker.enter("seat");
                    log.record("ate");
                    jitter(2);
                    tracker.exit("seat");
                });
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("seat") <= 5,
                    "more diners than seats under load: " +
                        std::to_string(tracker.max_concurrent("seat")));
    KOAN_ASSERT_EQ(log.count("ate"), std::size_t{200});
}
