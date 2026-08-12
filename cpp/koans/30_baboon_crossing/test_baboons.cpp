#include "koan_test.hpp"
#include "baboons.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

namespace {

void enter(Rope& r, const std::string& direction) {
    if (direction == "east") r.east_enter(); else r.west_enter();
}

void exit_rope(Rope& r, const std::string& direction) {
    if (direction == "east") r.east_exit(); else r.west_exit();
}

}  // namespace

KOAN_TEST(directions_never_mix) {
    Rope rope;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 6; ++t) {
        for (std::string direction : {"east", "west"}) {
            std::string other = (direction == "east") ? "west" : "east";
            runner.spawn([&, direction, other] {
                for (int i = 0; i < 15; ++i) {
                    jitter();
                    enter(rope, direction);
                    auto snapshot = tracker.enter(direction);
                    if (snapshot[other] > 0)
                        tracker.violate("an " + direction +
                                        "bound baboon got on while " +
                                        std::to_string(snapshot[other]) + " " +
                                        other + "bound were on the rope");
                    jitter(1);
                    tracker.exit(direction);
                    exit_rope(rope, direction);
                }
            });
        }
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
}

KOAN_TEST(rope_holds_at_most_five) {
    Rope rope(5);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 8; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 8; ++i) {
                rope.east_enter();
                tracker.enter("east");
                jitter(1);
                tracker.exit("east");
                rope.east_exit();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("east") <= 5,
                    "the rope snapped: " +
                        std::to_string(tracker.max_concurrent("east")) +
                        " baboons at once");
}

// One-baboon-at-a-time is not a crossing protocol; 5 must fit.
KOAN_TEST(same_direction_shares) {
    Rope rope(5);
    OverlapTracker tracker;
    std::atomic<bool> all_on{false};
    ThreadRunner runner;
    for (int t = 0; t < 5; ++t) {
        runner.spawn([&] {
            rope.east_enter();
            tracker.enter("east");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!all_on.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);
            tracker.exit("east");
            rope.east_exit();
        });
    }
    eventually([&] { return tracker.current("east") == 5; }, 5000ms,
               "5 same-direction baboons never shared the rope — "
               "over-serialized");
    all_on.store(true);
    runner.join_all(5000ms);
}

// With 5 on the rope, #6 waits for an exit — not for an empty rope.
KOAN_TEST(capacity_frees_slots) {
    auto rope = std::make_shared<Rope>(5);
    for (int i = 0; i < 5; ++i)
        assert_completes([rope] { rope->east_enter(); }, 2000ms,
                         "an in-capacity eastbound entry");
    auto probe = assert_blocks([rope] { rope->east_enter(); }, 300ms,
                               "baboon #6 (rope is full)");
    rope->east_exit();
    probe.assert_completed(5000ms, "baboon #6 after a slot freed");
    for (int i = 0; i < 5; ++i) rope->east_exit();
}

namespace {

void fairness_trial() {
    auto rope = std::make_shared<Rope>();
    auto log = std::make_shared<EventLog>();

    // Two eastbound incumbents get on and stay (main thread lets them off).
    for (int i = 0; i < 2; ++i)
        assert_completes([rope] { rope->east_enter(); }, 2000ms,
                         "an incumbent eastbound entry");

    // A westbound baboon arrives and must wait its turn.
    auto probe = assert_blocks(
        [rope, log] {
            rope->west_enter();
            log->record("west_on");
            rope->west_exit();
        },
        300ms, "the westbound baboon (eastbound owns the rope)");

    // Three more eastbound baboons arrive AFTER the westbound one.
    for (int i = 0; i < 3; ++i) {
        std::thread([rope, log] {
            rope->east_enter();
            log->record("late_east_on");
            rope->east_exit();
        }).detach();
    }
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("late_east_on") == 0,
                    "eastbound baboons that arrived after the waiting "
                    "westbound one overtook it: " + log->joined());

    // Incumbents finish crossing; the westbound waiter goes next.
    for (int i = 0; i < 2; ++i) rope->east_exit();
    probe.assert_completed(5000ms, "the waiting westbound baboon");
    log->wait_for_count("late_east_on", 3, 5000ms);
    log->assert_before("west_on", "late_east_on");
}

}  // namespace

KOAN_TEST(waiting_westbound_beats_late_eastbound) {
    for (int i = 0; i < 5; ++i) fairness_trial();
}

KOAN_TEST(stress_mixed_traffic) {
    Rope rope;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 4; ++t) {
        for (std::string direction : {"east", "west"}) {
            std::string other = (direction == "east") ? "west" : "east";
            runner.spawn([&, direction, other] {
                for (int i = 0; i < 10; ++i) {
                    jitter();
                    enter(rope, direction);
                    auto snapshot = tracker.enter(direction);
                    if (snapshot[other] > 0)
                        tracker.violate(direction + " and " + other +
                                        " on the rope together");
                    if (snapshot[direction] + snapshot[other] > 5)
                        tracker.violate("rope overloaded");
                    jitter(1);
                    tracker.exit(direction);
                    exit_rope(rope, direction);
                }
            });
        }
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
    KOAN_ASSERT(tracker.max_combined() <= 5);
}
