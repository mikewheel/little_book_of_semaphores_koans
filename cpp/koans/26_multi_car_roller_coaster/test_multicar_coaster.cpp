#include "koan_test.hpp"
#include "multicar_coaster.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

constexpr int kCars = 3;
constexpr int kCapacity = 2;

struct Bench {
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();
    std::shared_ptr<MultiCarCoaster> coaster;

    Bench() {
        auto l = log;
        auto tag = [l](const char* what) {
            return [l, what](int id) {
                jitter(2);
                l->record(std::string(what) + ":" + std::to_string(id));
            };
        };
        coaster = std::make_shared<MultiCarCoaster>(
            kCars, kCapacity,
            MultiCarHooks{tag("load"), tag("run"), tag("unload"),
                          tag("board"), tag("unboard")});
    }

    std::vector<int> ids_of(const std::string& prefix) const {
        std::vector<int> out;
        for (const auto& e : log->events())
            if (e.rfind(prefix, 0) == 0)
                out.push_back(std::stoi(e.substr(prefix.size())));
        return out;
    }
};

std::vector<int> expected_rotation(int rides_per_car) {
    std::vector<int> out;
    for (int r = 0; r < rides_per_car; ++r)
        for (int c = 0; c < kCars; ++c) out.push_back(c);
    return out;
}

// Each load opens a boarding window; exactly `capacity` boards must land
// inside it before any other car may call load.
void assert_one_car_boarding_at_a_time(const std::vector<std::string>& events,
                                       int capacity) {
    int open_car = -1;
    int boards_in_window = 0;
    for (const auto& e : events) {
        if (e.rfind("load:", 0) == 0) {
            int car = std::stoi(e.substr(5));
            KOAN_ASSERT_MSG(open_car == -1,
                            "car " + std::to_string(car) +
                                " called load while car " +
                                std::to_string(open_car) + " was still boarding");
            open_car = car;
            boards_in_window = 0;
        } else if (e.rfind("board:", 0) == 0) {
            KOAN_ASSERT_MSG(open_car != -1,
                            "a passenger boarded outside any boarding window");
            if (++boards_in_window == capacity) open_car = -1;  // window done
        }
    }
    KOAN_ASSERT_MSG(open_car == -1, "car " + std::to_string(open_car) +
                                        "'s boarding window never filled");
}

// Each unload is followed by exactly `capacity` unboards before the next.
void assert_unloads_are_whole(const std::vector<std::string>& events,
                              int capacity) {
    int open_car = -1;
    int unboards_in_window = 0;
    for (const auto& e : events) {
        if (e.rfind("unload:", 0) == 0) {
            int car = std::stoi(e.substr(7));
            KOAN_ASSERT_MSG(open_car == -1,
                            "car " + std::to_string(car) +
                                " called unload while car " +
                                std::to_string(open_car) +
                                "'s riders were still getting off");
            open_car = car;
            unboards_in_window = 0;
        } else if (e.rfind("unboard:", 0) == 0) {
            KOAN_ASSERT_MSG(open_car != -1,
                            "a passenger unboarded outside any unloading window");
            if (++unboards_in_window == capacity) open_car = -1;
        }
    }
    KOAN_ASSERT_MSG(open_car == -1, "car " + std::to_string(open_car) +
                                        "'s unloading window never emptied");
}

Bench run_park(int rides_per_car,
               std::chrono::milliseconds join_timeout = 15000ms) {
    const int n_passengers = kCars * kCapacity * rides_per_car;
    Bench bench;
    ThreadRunner runner;
    auto coaster = bench.coaster;
    runner.spawn([coaster, rides_per_car] { coaster->start_cars(rides_per_car); },
                 "cars");
    for (int pid = 0; pid < n_passengers; ++pid) {
        runner.spawn(
            [coaster, pid] {
                jitter();
                coaster->passenger(pid);
            },
            "passenger-" + std::to_string(pid));
    }
    runner.join_all(join_timeout);
    return bench;
}

std::vector<int> sorted(std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}

std::vector<int> iota_vec(int n) {
    std::vector<int> v(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) v[static_cast<std::size_t>(i)] = i;
    return v;
}

}  // namespace

KOAN_TEST(loads_rotate_in_order) {
    auto bench = run_park(2);
    KOAN_ASSERT_MSG(bench.ids_of("load:") == expected_rotation(2),
                    "cars must load in rotation 0,1,2,0,1,2 — log: " +
                        bench.log->joined());
}

KOAN_TEST(unloads_match_load_order) {
    auto bench = run_park(2);
    KOAN_ASSERT_MSG(bench.ids_of("unload:") == bench.ids_of("load:"),
                    "cars cannot pass each other: unload order must equal "
                    "load order — log: " +
                        bench.log->joined());
}

KOAN_TEST(one_car_boarding_at_a_time) {
    auto bench = run_park(2);
    assert_one_car_boarding_at_a_time(bench.log->events(), kCapacity);
}

KOAN_TEST(all_passengers_ride) {
    auto bench = run_park(2);
    KOAN_ASSERT_MSG(sorted(bench.ids_of("board:")) == iota_vec(12),
                    "every passenger must board exactly once");
    KOAN_ASSERT(sorted(bench.ids_of("unboard:")) == iota_vec(12));
    KOAN_ASSERT_EQ(bench.ids_of("run:").size(), std::size_t{6});
}

KOAN_TEST(stress) {
    auto bench = run_park(3, 20000ms);
    auto events = bench.log->events();
    KOAN_ASSERT(bench.ids_of("load:") == expected_rotation(3));
    KOAN_ASSERT(bench.ids_of("unload:") == expected_rotation(3));
    assert_one_car_boarding_at_a_time(events, kCapacity);
    assert_unloads_are_whole(events, kCapacity);
    KOAN_ASSERT(sorted(bench.ids_of("board:")) == iota_vec(18));
}
