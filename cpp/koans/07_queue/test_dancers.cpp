#include "koan_test.hpp"
#include "dancers.hpp"

#include <memory>
#include <thread>

using namespace koans;

KOAN_TEST(lone_leader_blocks) {
    auto floor = std::make_shared<DanceFloor>();
    auto probe = assert_blocks([floor] { floor->leader_arrives(); }, 300ms,
                               "leader_arrives (no follower yet)");
    std::thread([floor] { floor->follower_arrives(); }).detach();
    probe.assert_completed(5000ms, "the parked leader once a follower arrives");
}

KOAN_TEST(lone_follower_blocks) {
    auto floor = std::make_shared<DanceFloor>();
    auto probe = assert_blocks([floor] { floor->follower_arrives(); }, 300ms,
                               "follower_arrives (no leader yet)");
    std::thread([floor] { floor->leader_arrives(); }).detach();
    probe.assert_completed(5000ms, "the parked follower once a leader arrives");
}

KOAN_TEST(pair_completes) {
    for (int i = 0; i < 20; ++i) {
        auto floor = std::make_shared<DanceFloor>();
        ThreadRunner runner;
        runner.spawn([floor] { floor->leader_arrives(); }, "leader");
        runner.spawn([floor] { floor->follower_arrives(); }, "follower");
        runner.join_all(5000ms);
    }
}

KOAN_TEST(three_leaders_one_follower) {
    auto floor = std::make_shared<DanceFloor>();
    auto done = std::make_shared<EventLog>();
    ThreadRunner runner;
    auto leader = [floor, done] {
        floor->leader_arrives();
        done->record("leader");
    };
    auto follower = [floor, done] {
        floor->follower_arrives();
        done->record("follower");
    };
    for (int i = 0; i < 3; ++i) runner.spawn(leader, "leader");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(done->count("leader") == 0,
                    "no leader may proceed before any follower arrives");

    runner.spawn(follower, "follower");
    eventually(
        [&] { return done->count("leader") == 1 && done->count("follower") == 1; },
        5000ms, "one follower should release exactly one pair");
    std::this_thread::sleep_for(300ms);  // a window for surplus leaders
    KOAN_ASSERT_MSG(done->count("leader") == 1,
                    "one follower released " +
                        std::to_string(done->count("leader")) + " leaders");

    runner.spawn(follower, "follower");
    runner.spawn(follower, "follower");
    runner.join_all(5000ms);
    KOAN_ASSERT_EQ(done->count("leader"), static_cast<std::size_t>(3));
    KOAN_ASSERT_EQ(done->count("follower"), static_cast<std::size_t>(3));
}

KOAN_TEST(balanced_stress) {
    auto floor = std::make_shared<DanceFloor>();
    auto done = std::make_shared<EventLog>();
    ThreadRunner runner;
    for (int i = 0; i < 30; ++i) {
        runner.spawn(
            [floor, done] {
                jitter(3);
                floor->leader_arrives();
                done->record("leader");
            },
            "leader");
        runner.spawn(
            [floor, done] {
                jitter(3);
                floor->follower_arrives();
                done->record("follower");
            },
            "follower");
    }
    runner.join_all(15000ms);
    KOAN_ASSERT_EQ(done->count("leader"), static_cast<std::size_t>(30));
    KOAN_ASSERT_EQ(done->count("follower"), static_cast<std::size_t>(30));
}
