#include "koan_test.hpp"
#include "bathroom.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

namespace {

constexpr int kCapacity = 3;

void gendered_loop(Bathroom& bathroom, OverlapTracker& tracker,
                   const std::string& me, const std::string& other,
                   int iterations = 15) {
    const bool female = me == "female";
    for (int k = 0; k < iterations; ++k) {
        jitter();
        if (female)
            bathroom.female_enter();
        else
            bathroom.male_enter();
        auto snap = tracker.enter(me);
        if (snap[other] > 0)
            tracker.violate("a " + me + " entered while " +
                            std::to_string(snap[other]) + " " + other +
                            "(s) were inside");
        if (snap[me] > kCapacity)
            tracker.violate("more than " + std::to_string(kCapacity) + " " +
                            me + "s inside");
        jitter(1);
        tracker.exit(me);
        if (female)
            bathroom.female_exit();
        else
            bathroom.male_exit();
    }
}

}  // namespace

KOAN_TEST(genders_never_mix) {
    Bathroom bathroom(kCapacity);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int i = 0; i < 6; ++i) {
        runner.spawn([&] { gendered_loop(bathroom, tracker, "female", "male"); });
        runner.spawn([&] { gendered_loop(bathroom, tracker, "male", "female"); });
    }
    runner.join_all(20000ms);
    tracker.assert_no_violations();
}

KOAN_TEST(capacity_respected) {
    Bathroom bathroom(kCapacity);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int i = 0; i < 6; ++i) {
        runner.spawn([&] {
            for (int k = 0; k < 10; ++k) {
                bathroom.female_enter();
                tracker.enter("female");
                jitter(1);
                tracker.exit("female");
                bathroom.female_exit();
            }
        });
    }
    runner.join_all(20000ms);
    KOAN_ASSERT_MSG(tracker.max_concurrent("female") <= kCapacity,
                    "capacity " + std::to_string(kCapacity) + " exceeded: saw " +
                        std::to_string(tracker.max_concurrent("female")) +
                        " women inside at once");
}

KOAN_TEST(same_gender_shares) {
    Bathroom bathroom(kCapacity);
    OverlapTracker tracker;
    std::atomic<bool> release{false};
    ThreadRunner runner;
    for (int i = 0; i < 3; ++i) {
        runner.spawn([&] {
            bathroom.female_enter();
            tracker.enter("female");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!release.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);  // linger until all 3 inside
            tracker.exit("female");
            bathroom.female_exit();
        });
    }
    eventually([&] { return tracker.current("female") == 3; }, 5000ms,
               "not all 3 women made it inside together — same-gender "
               "sharing up to capacity is required");
    release.store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(opposite_blocks_until_empty) {
    auto bathroom = std::make_shared<Bathroom>(kCapacity);
    assert_completes([bathroom] { bathroom->female_enter(); }, 2000ms,
                     "the first woman's entry");
    assert_completes([bathroom] { bathroom->female_enter(); }, 2000ms,
                     "the second woman's entry");

    auto probe = assert_blocks([bathroom] { bathroom->male_enter(); }, 300ms,
                               "a man (women are inside)");
    bathroom->female_exit();  // one woman leaves; the room is NOT empty yet
    KOAN_ASSERT_MSG(!probe.wait(300ms),
                    "the man entered as soon as a slot freed — he may only "
                    "enter once the bathroom is completely EMPTY");
    bathroom->female_exit();  // the last woman leaves
    probe.assert_completed(5000ms, "the man once the bathroom is empty");
    bathroom->male_exit();
}

KOAN_TEST(stress) {
    Bathroom bathroom(kCapacity);
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int i = 0; i < 8; ++i) {
        runner.spawn(
            [&] { gendered_loop(bathroom, tracker, "female", "male", 20); });
        runner.spawn(
            [&] { gendered_loop(bathroom, tracker, "male", "female", 20); });
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
    KOAN_ASSERT_MSG(tracker.max_combined() <= kCapacity,
                    "more than " + std::to_string(kCapacity) +
                        " people inside at once: " +
                        std::to_string(tracker.max_combined()));
}
