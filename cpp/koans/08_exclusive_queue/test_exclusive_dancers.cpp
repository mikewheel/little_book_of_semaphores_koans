#include "koan_test.hpp"
#include "exclusive_dancers.hpp"

#include <memory>
#include <thread>

using namespace koans;

KOAN_TEST(lone_leader_blocks) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [floor, log] {
            floor->leader_dances([&] { log->record("leader_danced"); });
        },
        300ms, "leader_dances (no follower yet)");
    KOAN_ASSERT_MSG(log->count("leader_danced") == 0,
                    "the lone leader's dance ran anyway");
    std::thread([floor, log] {
        floor->follower_dances([&] { log->record("follower_danced"); });
    }).detach();
    probe.assert_completed(5000ms, "the parked leader once a follower arrives");
    KOAN_ASSERT_EQ(log->count("leader_danced"), static_cast<std::size_t>(1));
}

KOAN_TEST(lone_follower_blocks) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [floor, log] {
            floor->follower_dances([&] { log->record("follower_danced"); });
        },
        300ms, "follower_dances (no leader yet)");
    KOAN_ASSERT_MSG(log->count("follower_danced") == 0,
                    "the lone follower's dance ran anyway");
    std::thread([floor, log] {
        floor->leader_dances([&] { log->record("leader_danced"); });
    }).detach();
    probe.assert_completed(5000ms, "the parked follower once a leader arrives");
    KOAN_ASSERT_EQ(log->count("follower_danced"), static_cast<std::size_t>(1));
}

KOAN_TEST(pair_dances_together) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto tracker = std::make_shared<OverlapTracker>();
    auto log = std::make_shared<EventLog>();
    ThreadRunner runner;
    runner.spawn(
        [floor, tracker, log] {
            floor->leader_dances([&] {
                tracker->enter("leader_dance");
                log->record("enter:leader");
                // Stay on the floor until the partner demonstrably joined it.
                log->wait_for_count("enter:follower", 1, 5000ms);
                tracker->exit("leader_dance");
            });
        },
        "leader");
    runner.spawn(
        [floor, tracker, log] {
            floor->follower_dances([&] {
                tracker->enter("follower_dance");
                log->record("enter:follower");
                log->wait_for_count("enter:leader", 1, 5000ms);
                tracker->exit("follower_dance");
            });
        },
        "follower");
    runner.join_all(10000ms);
    KOAN_ASSERT_MSG(tracker->max_combined() == 2,
                    "the pair's two dance callbacks never overlapped — "
                    "partners must dance together");
}

KOAN_TEST(leader_returns_only_after_partners_dance_ends) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    for (int rep = 0; rep < 2; ++rep) {
        auto log = std::make_shared<EventLog>();
        ThreadRunner runner;
        runner.spawn(
            [floor, log] {
                floor->follower_dances([&] {
                    log->record("follower_start");
                    std::this_thread::sleep_for(150ms);
                    log->record("follower_end");
                });
            },
            "follower");
        std::this_thread::sleep_for(50ms);  // let the follower park
        assert_completes(
            [floor, log] {
                floor->leader_dances([&] { log->record("leader_danced"); });
            },
            5000ms, "leader_dances");
        KOAN_ASSERT_MSG(log->count("follower_end") == 1,
                        "leader_dances() returned while its partner was "
                        "still dancing");
        runner.join_all(5000ms);
    }
}

KOAN_TEST(parked_leader_still_waits_for_its_partner) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [floor, log] {
            floor->leader_dances([&] { log->record("leader_danced"); });
        },
        300ms, "leader_dances (no follower yet)");
    ThreadRunner runner;
    runner.spawn(
        [floor, log] {
            floor->follower_dances([&] {
                log->record("follower_start");
                std::this_thread::sleep_for(150ms);
                log->record("follower_end");
            });
        },
        "follower");
    probe.assert_completed(5000ms, "the parked leader once the pair danced");
    KOAN_ASSERT_MSG(log->count("follower_end") == 1,
                    "the leader returned before its partner finished dancing");
    runner.join_all(5000ms);
}

KOAN_TEST(at_most_one_pair_on_floor) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto tracker = std::make_shared<OverlapTracker>();
    ThreadRunner runner;
    for (int i = 0; i < 6; ++i) {
        runner.spawn(
            [floor, tracker] {
                jitter(3);
                floor->leader_dances([&] {
                    tracker->enter("leader_dance");
                    std::this_thread::sleep_for(10ms);
                    jitter();
                    tracker->exit("leader_dance");
                });
            },
            "leader");
        runner.spawn(
            [floor, tracker] {
                jitter(3);
                floor->follower_dances([&] {
                    tracker->enter("follower_dance");
                    std::this_thread::sleep_for(10ms);
                    jitter();
                    tracker->exit("follower_dance");
                });
            },
            "follower");
    }
    runner.join_all(15000ms);
    KOAN_ASSERT_MSG(tracker->max_concurrent("leader_dance") == 1,
                    std::to_string(tracker->max_concurrent("leader_dance")) +
                        " leaders danced at once");
    KOAN_ASSERT_MSG(tracker->max_concurrent("follower_dance") == 1,
                    std::to_string(tracker->max_concurrent("follower_dance")) +
                        " followers danced at once");
    KOAN_ASSERT_MSG(tracker->max_combined() <= 2,
                    std::to_string(tracker->max_combined()) +
                        " dancers shared the floor — that is more than one pair");
}

KOAN_TEST(all_complete_stress) {
    auto floor = std::make_shared<ExclusiveDanceFloor>();
    auto done = std::make_shared<EventLog>();
    ThreadRunner runner;
    for (int i = 0; i < 20; ++i) {
        runner.spawn(
            [floor, done] {
                jitter(3);
                floor->leader_dances([&] { done->record("leader_danced"); });
                done->record("leader_returned");
            },
            "leader");
        runner.spawn(
            [floor, done] {
                jitter(3);
                floor->follower_dances([&] { done->record("follower_danced"); });
                done->record("follower_returned");
            },
            "follower");
    }
    runner.join_all(20000ms);
    KOAN_ASSERT_EQ(done->count("leader_danced"), static_cast<std::size_t>(20));
    KOAN_ASSERT_EQ(done->count("follower_danced"), static_cast<std::size_t>(20));
    KOAN_ASSERT_EQ(done->count("leader_returned"), static_cast<std::size_t>(20));
    KOAN_ASSERT_EQ(done->count("follower_returned"), static_cast<std::size_t>(20));
}
