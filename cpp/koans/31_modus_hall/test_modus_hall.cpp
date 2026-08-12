#include "koan_test.hpp"
#include "modus_hall.hpp"

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <thread>

using namespace koans;

KOAN_TEST(factions_never_mix) {
    Path path;
    OverlapTracker tracker;
    ThreadRunner runner;
    for (int t = 0; t < 8; ++t) {
        for (std::string faction : {"heathen", "prude"}) {
            std::string other = (faction == "heathen") ? "prude" : "heathen";
            runner.spawn([&, faction, other] {
                auto cross = [&] {
                    auto snapshot = tracker.enter(faction);
                    if (snapshot[other] > 0)
                        tracker.violate("a " + faction +
                                        " was on the path with " +
                                        std::to_string(snapshot[other]) + " " +
                                        other + "(s)");
                    std::this_thread::sleep_for(1ms);
                    jitter(2);
                    tracker.exit(faction);
                };
                for (int i = 0; i < 10; ++i) {
                    jitter();
                    if (faction == "heathen") path.heathen_cross(cross);
                    else path.prude_cross(cross);
                }
            });
        }
    }
    runner.join_all(30000ms);
    tracker.assert_no_violations();
}

KOAN_TEST(first_arrival_claims_field) {
    auto path = std::make_shared<Path>();
    auto log = std::make_shared<EventLog>();
    assert_completes(
        [path, log] { path->heathen_cross([log] { log->record("crossed"); }); },
        2000ms, "a lone heathen's crossing of an empty path");
    KOAN_ASSERT_EQ(log->count("crossed"), std::size_t{1});
}

KOAN_TEST(same_faction_shares) {
    Path path;
    OverlapTracker tracker;
    std::atomic<bool> all_on{false};
    ThreadRunner runner;
    for (int t = 0; t < 4; ++t) {
        runner.spawn([&] {
            path.heathen_cross([&] {
                tracker.enter("heathen");
                auto deadline = std::chrono::steady_clock::now() + 5s;
                while (!all_on.load() &&
                       std::chrono::steady_clock::now() < deadline)
                    std::this_thread::sleep_for(1ms);
                tracker.exit("heathen");
            });
        });
    }
    eventually([&] { return tracker.current("heathen") == 4; }, 5000ms,
               "4 heathens never shared the path — the same faction must "
               "not block itself");
    all_on.store(true);
    runner.join_all(5000ms);
}

namespace {

void majority_trial() {
    auto path = std::make_shared<Path>();
    auto log = std::make_shared<EventLog>();
    auto hold = std::make_shared<std::counting_semaphore<>>(0);

    // Two heathens take the path and stay on it. The first goes through
    // assert_blocks so an unimplemented/broken solution fails fast.
    auto incumbent = [path, log, hold] {
        path->heathen_cross([log, hold] {
            log->record("heathen_on_path");
            (void)hold->try_acquire_for(10s);
        });
    };
    auto inc_probe = assert_blocks(incumbent, 300ms,
                                   "an incumbent heathen (holding the path)");
    std::thread(incumbent).detach();
    log->wait_for_count("heathen_on_path", 2, 5000ms);

    // Three prudes arrive: 3 waiting prudes > 2 crossing heathens — the
    // balance tips to the prudes.
    for (int i = 0; i < 3; ++i) {
        std::thread([path, log] {
            log->record("prude_arrived");
            path->prude_cross([log] { log->record("prude_on_path"); });
        }).detach();
    }
    log->wait_for_count("prude_arrived", 3, 5000ms);
    std::this_thread::sleep_for(300ms);  // let all three check in and tip
    KOAN_ASSERT_MSG(log->count("prude_on_path") == 0,
                    "prudes crossed while heathens were still on the path");

    // A heathen arriving after the tip must NOT cross before the prude batch.
    auto late_probe = assert_blocks(
        [path, log] {
            path->heathen_cross([log] { log->record("late_heathen_on_path"); });
        },
        300ms, "a heathen arriving after the prudes gained majority");

    // Incumbents finish: the whole prude cohort crosses, then the heathen.
    hold->release(2);
    log->wait_for_count("prude_on_path", 3, 5000ms);
    late_probe.assert_completed(5000ms, "the late heathen's crossing");
    inc_probe.assert_completed(5000ms, "the incumbents' crossings");
    log->assert_before("prude_on_path", "late_heathen_on_path");
}

}  // namespace

KOAN_TEST(majority_flips_the_field) {
    for (int i = 0; i < 5; ++i) majority_trial();
}

KOAN_TEST(minority_keeps_waiting) {
    auto path = std::make_shared<Path>();
    auto log = std::make_shared<EventLog>();
    auto hold = std::make_shared<std::counting_semaphore<>>(0);

    // Three heathens hold the path.
    auto incumbent = [path, log, hold] {
        path->heathen_cross([log, hold] {
            log->record("heathen_on_path");
            (void)hold->try_acquire_for(10s);
        });
    };
    auto inc_probe = assert_blocks(incumbent, 300ms,
                                   "an incumbent heathen (holding the path)");
    for (int i = 0; i < 2; ++i) std::thread(incumbent).detach();
    log->wait_for_count("heathen_on_path", 3, 5000ms);

    // Two prudes arrive: 2 < 3, no majority — they must wait.
    for (int i = 0; i < 2; ++i) {
        std::thread([path, log] {
            log->record("prude_arrived");
            path->prude_cross([log] { log->record("prude_on_path"); });
        }).detach();
    }
    log->wait_for_count("prude_arrived", 2, 5000ms);
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("prude_on_path") == 0,
                    "an outnumbered prude crossed");

    // Heathen #4 strolls through: his faction still rules.
    assert_completes(
        [path, log] {
            path->heathen_cross([log] { log->record("h4_on_path"); });
        },
        2000ms,
        "a free pass for a heathen while his faction holds the path and "
        "the opposition lacks a majority");
    KOAN_ASSERT_MSG(log->count("prude_on_path") == 0,
                    "prudes crossed while still outnumbered");

    // Cleanup: incumbents leave; the prude pair finally gets the path.
    hold->release(3);
    log->wait_for_count("prude_on_path", 2, 5000ms);
    inc_probe.assert_completed(5000ms, "the incumbents' crossings");
}

KOAN_TEST(everyone_eventually_crosses) {
    Path path;
    EventLog log;
    ThreadRunner runner;
    for (int t = 0; t < 8; ++t) {
        for (std::string faction : {"h", "p"}) {
            runner.spawn([&, faction] {
                for (int i = 0; i < 5; ++i) {
                    jitter();
                    auto cross = [&] {
                        log.record(faction);
                        jitter(1);
                    };
                    if (faction == "h") path.heathen_cross(cross);
                    else path.prude_cross(cross);
                }
            });
        }
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_EQ(log.count("h"), std::size_t{40});
    KOAN_ASSERT_EQ(log.count("p"), std::size_t{40});
}
