#include "koan_test.hpp"
#include "fair_bathroom.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

namespace {

void enter(FairBathroom& b, const std::string& gender) {
    if (gender == "male") b.male_enter(); else b.female_enter();
}

void exit_room(FairBathroom& b, const std::string& gender) {
    if (gender == "male") b.male_exit(); else b.female_exit();
}

}  // namespace

KOAN_TEST(genders_never_mix) {
    FairBathroom bathroom;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 6; ++t) {
        for (std::string gender : {"male", "female"}) {
            std::string other = (gender == "male") ? "female" : "male";
            runner.spawn([&, gender, other] {
                for (int i = 0; i < 15; ++i) {
                    jitter();
                    enter(bathroom, gender);
                    auto snapshot = tracker.enter(gender);
                    if (snapshot[other] > 0)
                        tracker.violate("a " + gender + " entered while " +
                                        std::to_string(snapshot[other]) + " " +
                                        other + "(s) inside");
                    jitter(1);
                    tracker.exit(gender);
                    exit_room(bathroom, gender);
                }
            });
        }
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
}

KOAN_TEST(capacity_respected) {
    FairBathroom bathroom(3);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 6; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < 10; ++i) {
                bathroom.male_enter();
                tracker.enter("inside");
                jitter(1);
                tracker.exit("inside");
                bathroom.male_exit();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("inside") <= 3,
                    "capacity 3 exceeded: saw " +
                        std::to_string(tracker.max_concurrent("inside")));
}

// A room that admits one person at a time is safe but wrong.
KOAN_TEST(same_gender_shares_up_to_capacity) {
    FairBathroom bathroom(3);
    OverlapTracker tracker;
    std::atomic<bool> all_in{false};
    ThreadRunner runner;
    for (int t = 0; t < 3; ++t) {
        runner.spawn([&] {
            bathroom.female_enter();
            tracker.enter("female");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!all_in.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);
            tracker.exit("female");
            bathroom.female_exit();
        });
    }
    eventually([&] { return tracker.current("female") == 3; }, 5000ms,
               "3 women never shared the bathroom — the room is "
               "over-serialized");
    all_in.store(true);
    runner.join_all(5000ms);
}

// A man enters only when the room is EMPTY, not when a slot frees up.
KOAN_TEST(opposite_gender_blocks_until_empty) {
    auto bathroom = std::make_shared<FairBathroom>();
    for (int i = 0; i < 2; ++i)
        assert_completes([bathroom] { bathroom->female_enter(); }, 2000ms,
                         "a woman's entry into an empty room");
    auto probe = assert_blocks([bathroom] { bathroom->male_enter(); }, 300ms,
                               "male_enter while women are inside");
    bathroom->female_exit();
    KOAN_ASSERT_MSG(!probe.wait(250ms),
                    "one woman left but another is still inside — the man "
                    "must keep waiting");
    bathroom->female_exit();
    probe.assert_completed(5000ms, "the waiting man (room now empty)");
    bathroom->male_exit();
}

namespace {

void fairness_trial(const std::string& waiter_gender,
                    const std::string& incumbent_gender) {
    auto bathroom = std::make_shared<FairBathroom>();
    auto log = std::make_shared<EventLog>();

    // Two incumbents settle in and stay (main thread will let them out).
    for (int i = 0; i < 2; ++i)
        assert_completes(
            [bathroom, incumbent_gender] { enter(*bathroom, incumbent_gender); },
            2000ms, "an incumbent's entry");

    // The waiter arrives and must block: the room belongs to the others.
    auto probe = assert_blocks(
        [bathroom, log, waiter_gender] {
            enter(*bathroom, waiter_gender);
            log->record("waiter_in");
            exit_room(*bathroom, waiter_gender);
        },
        300ms, "the " + waiter_gender + "'s entry while " + incumbent_gender +
                   "s are inside");

    // Three more of the incumbent gender arrive AFTER the waiter.
    for (int i = 0; i < 3; ++i) {
        std::thread([bathroom, log, incumbent_gender] {
            enter(*bathroom, incumbent_gender);
            log->record("late_in");
            exit_room(*bathroom, incumbent_gender);
        }).detach();
    }
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("late_in") == 0,
                    incumbent_gender + "s who arrived after the waiting " +
                        waiter_gender + " slipped in ahead: " + log->joined());

    // The incumbents leave; the waiter must be served before the latecomers.
    for (int i = 0; i < 2; ++i) exit_room(*bathroom, incumbent_gender);
    probe.assert_completed(5000ms, "the waiting " + waiter_gender);
    log->wait_for_count("late_in", 3, 5000ms);
    log->assert_before("waiter_in", "late_in");
}

}  // namespace

KOAN_TEST(waiting_male_beats_late_females) {
    for (int i = 0; i < 5; ++i) fairness_trial("male", "female");
}

KOAN_TEST(waiting_female_beats_late_males) {
    for (int i = 0; i < 5; ++i) fairness_trial("female", "male");
}

// A continuous parade of women must not shut men out indefinitely.
KOAN_TEST(steady_stream_cannot_starve) {
    auto bathroom = std::make_shared<FairBathroom>();
    auto stop = std::make_shared<std::atomic<bool>>(false);
    ThreadRunner runner;
    for (int t = 0; t < 3; ++t) {
        runner.spawn([bathroom, stop] {
            while (!stop->load()) {
                bathroom->female_enter();
                std::this_thread::sleep_for(2ms);
                bathroom->female_exit();
            }
        });
    }
    std::this_thread::sleep_for(100ms);  // let the stream establish itself
    try {
        assert_completes(
            [bathroom] {
                bathroom->male_enter();
                bathroom->male_exit();
            },
            4000ms, "a man's visit while women stream through");
    } catch (...) {
        stop->store(true);
        throw;
    }
    stop->store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(stress_mixed_traffic) {
    FairBathroom bathroom;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 4; ++t) {
        for (std::string gender : {"male", "female"}) {
            std::string other = (gender == "male") ? "female" : "male";
            runner.spawn([&, gender, other] {
                for (int i = 0; i < 10; ++i) {
                    jitter();
                    enter(bathroom, gender);
                    auto snapshot = tracker.enter(gender);
                    if (snapshot[other] > 0)
                        tracker.violate(gender + " and " + other +
                                        " inside together");
                    if (snapshot[gender] + snapshot[other] > 3)
                        tracker.violate("over capacity");
                    jitter(1);
                    tracker.exit(gender);
                    exit_room(bathroom, gender);
                }
            });
        }
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
    KOAN_ASSERT(tracker.max_combined() <= 3);
}
