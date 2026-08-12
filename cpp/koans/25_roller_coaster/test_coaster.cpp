#include "koan_test.hpp"
#include "coaster.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

constexpr int kCapacity = 4;

struct Bench {
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();
    std::shared_ptr<RollerCoaster> coaster;

    Bench() {
        auto l = log;
        coaster = std::make_shared<RollerCoaster>(
            kCapacity,
            CoasterHooks{
                [l] { jitter(2); l->record("load"); },
                [l] { jitter(2); l->record("run"); },
                [l] { jitter(2); l->record("unload"); },
                [l](int pid) { jitter(2); l->record("board:" + std::to_string(pid)); },
                [l](int pid) { jitter(2); l->record("unboard:" + std::to_string(pid)); },
            });
    }
};

// The exact sequence: (load, C boards, run, unload, C unboards) × rides.
void assert_cycle_pattern(const std::vector<std::string>& events, int capacity,
                          int rides) {
    std::size_t i = 0;
    auto expect = [&](const std::string& what, bool prefix) {
        KOAN_ASSERT_MSG(i < events.size(),
                        "log ended early; expected '" + what + "' at position " +
                            std::to_string(i));
        bool ok = prefix ? events[i].rfind(what, 0) == 0 : events[i] == what;
        KOAN_ASSERT_MSG(ok, "expected '" + what + "' at position " +
                                std::to_string(i) + " but saw '" + events[i] + "'");
        ++i;
    };
    for (int r = 0; r < rides; ++r) {
        expect("load", false);
        for (int c = 0; c < capacity; ++c) expect("board:", true);
        expect("run", false);
        expect("unload", false);
        for (int c = 0; c < capacity; ++c) expect("unboard:", true);
    }
    KOAN_ASSERT_MSG(i == events.size(), "unexpected extra events after ride " +
                                            std::to_string(rides));
}

std::vector<int> pids_of(const std::vector<std::string>& events,
                         const std::string& prefix) {
    std::vector<int> out;
    for (const auto& e : events)
        if (e.rfind(prefix, 0) == 0)
            out.push_back(std::stoi(e.substr(prefix.size())));
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<int> iota_vec(int n) {
    std::vector<int> v(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) v[static_cast<std::size_t>(i)] = i;
    return v;
}

Bench run_park(int n_passengers, int n_rides,
               std::chrono::milliseconds join_timeout = 15000ms) {
    Bench bench;
    ThreadRunner runner;
    auto rc = bench.coaster;
    runner.spawn([rc, n_rides] { rc->start_car(n_rides); }, "car");
    for (int pid = 0; pid < n_passengers; ++pid) {
        runner.spawn(
            [rc, pid] {
                jitter();
                rc->passenger(pid);
            },
            "passenger-" + std::to_string(pid));
    }
    runner.join_all(join_timeout);
    return bench;
}

}  // namespace

KOAN_TEST(cycle_ordering) {
    auto bench = run_park(4, 1, 10000ms);
    auto events = bench.log->events();
    assert_cycle_pattern(events, kCapacity, 1);
    KOAN_ASSERT(pids_of(events, "board:") == iota_vec(4));
    KOAN_ASSERT(pids_of(events, "unboard:") == iota_vec(4));
}

KOAN_TEST(car_waits_for_full_load) {
    Bench bench;
    ThreadRunner runner;
    auto rc = bench.coaster;
    runner.spawn([rc] { rc->start_car(1); }, "car");
    for (int pid = 0; pid < 3; ++pid)
        runner.spawn([rc, pid] { rc->passenger(pid); },
                     "passenger-" + std::to_string(pid));
    std::this_thread::sleep_for(400ms);  // 3 of 4 seats: tempt an early run()
    KOAN_ASSERT_MSG(bench.log->count("run") == 0,
                    "the car ran with only 3 of 4 passengers: " +
                        bench.log->joined());
    runner.spawn([rc] { rc->passenger(3); }, "passenger-3");  // the last seat
    runner.join_all(10000ms);
    assert_cycle_pattern(bench.log->events(), kCapacity, 1);
}

KOAN_TEST(passengers_wait_for_car) {
    Bench bench;
    ThreadRunner runner;
    auto rc = bench.coaster;
    for (int pid = 0; pid < 4; ++pid)
        runner.spawn([rc, pid] { rc->passenger(pid); },
                     "passenger-" + std::to_string(pid));
    std::this_thread::sleep_for(300ms);  // no car yet: nobody may board
    KOAN_ASSERT_MSG(bench.log->events().empty(),
                    "passengers boarded before the car called load(): " +
                        bench.log->joined());
    runner.spawn([rc] { rc->start_car(1); }, "car");
    runner.join_all(10000ms);
    assert_cycle_pattern(bench.log->events(), kCapacity, 1);
}

KOAN_TEST(multiple_rides) {
    auto bench = run_park(8, 2);
    auto events = bench.log->events();
    assert_cycle_pattern(events, kCapacity, 2);
    KOAN_ASSERT_MSG(pids_of(events, "board:") == iota_vec(8),
                    "every passenger must ride exactly once");
    KOAN_ASSERT(pids_of(events, "unboard:") == iota_vec(8));
}

KOAN_TEST(stress) {
    auto bench = run_park(12, 3, 20000ms);
    auto events = bench.log->events();
    assert_cycle_pattern(events, kCapacity, 3);
    KOAN_ASSERT(pids_of(events, "board:") == iota_vec(12));
}
