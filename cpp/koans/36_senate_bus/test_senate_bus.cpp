#include "koan_test.hpp"
#include "senate_bus.hpp"

#include <algorithm>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

constexpr int kCapacity = 5;

// A one-shot gate the board hook can hold on (10 s failsafe so no thread
// is ever stranded).
class Gate {
  public:
    void open() {
        {
            std::lock_guard lock(mutex_);
            open_ = true;
        }
        cond_.notify_all();
    }
    void await() {
        std::unique_lock lock(mutex_);
        cond_.wait_for(lock, 10s, [&] { return open_; });
    }

  private:
    std::mutex mutex_;
    std::condition_variable cond_;
    bool open_ = false;
};

struct Rig {
    std::shared_ptr<BusStop> stop;
    std::shared_ptr<EventLog> log;
    std::shared_ptr<Gate> gate;  // null unless the rig gates boarding
};

// The board hook logs board_begin:rid, optionally holds on the gate,
// then logs board:rid. The depart hook logs depart:n.
Rig make_rig(bool gated) {
    auto log = std::make_shared<EventLog>();
    auto gate = gated ? std::make_shared<Gate>() : nullptr;
    auto stop = std::make_shared<BusStop>(
        kCapacity,
        [log, gate](int rid) {
            log->record("board_begin:" + std::to_string(rid));
            if (gate) gate->await();
            log->record("board:" + std::to_string(rid));
        },
        [log](int n) { log->record("depart:" + std::to_string(n)); });
    return {stop, log, gate};
}

// Run fn on a detached thread without asserting anything about blocking.
// Everything fn touches is shared_ptr state captured by value.
template <typename F>
Probe spawn_probe(F fn) {
    Probe probe;
    std::thread([probe, fn = std::move(fn)]() mutable {
        try {
            fn();
        } catch (const std::exception& e) {
            *probe.error = e.what();
        } catch (...) {
            *probe.error = "non-exception thrown";
        }
        {
            std::lock_guard lock(*probe.mutex);
            probe.done->store(true);
        }
        probe.cond->notify_all();
    }).detach();
    return probe;
}

// Rider wrapper: logs the walk-up so tests can see arrivals even when
// rider() itself blocks.
Probe spawn_rider(const Rig& rig, int rid) {
    auto stop = rig.stop;
    auto log = rig.log;
    return spawn_probe([stop, log, rid] {
        log->record("at_stop:" + std::to_string(rid));
        stop->rider(rid);
    });
}

std::vector<std::string> boarders(const EventLog& log) {
    std::vector<std::string> out;
    for (const auto& e : log.events())
        if (e.rfind("board:", 0) == 0) out.push_back(e.substr(6));
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<int> departs(const EventLog& log) {
    std::vector<int> out;
    for (const auto& e : log.events())
        if (e.rfind("depart:", 0) == 0) out.push_back(std::stoi(e.substr(7)));
    return out;
}

int finished(const std::vector<Probe>& probes) {
    int n = 0;
    for (const auto& p : probes)
        if (p.done->load()) ++n;
    return n;
}

}  // namespace

KOAN_TEST(empty_stop_departs_immediately) {
    auto rig = make_rig(false);
    auto stop = rig.stop;
    assert_completes([stop] { stop->bus_arrives(); }, 2000ms,
                     "a bus at an empty stop (departs at once)");
    KOAN_ASSERT_MSG(rig.log->events() == std::vector<std::string>{"depart:0"},
                    "expected exactly depart(0) and no boarding; log " +
                        rig.log->joined());
    // A rider who shows up after that bus is gone must wait for the next.
    auto log = rig.log;
    auto probe = assert_blocks(
        [stop, log] {
            log->record("at_stop:1");
            stop->rider(1);
        },
        300ms, "a rider who missed the bus");
    assert_completes([stop] { stop->bus_arrives(); }, 2000ms,
                     "the next bus visit");
    probe.assert_completed(5000ms, "the waiting rider once the next bus comes");
    KOAN_ASSERT_MSG(
        rig.log->count("board:1") == 1 && rig.log->count("depart:1") == 1,
        "the second bus should take exactly rider 1; log " + rig.log->joined());
}

KOAN_TEST(waiting_riders_board) {
    auto rig = make_rig(false);
    auto stop = rig.stop;
    std::vector<Probe> probes;
    for (int rid = 0; rid < 3; ++rid) probes.push_back(spawn_rider(rig, rid));
    for (int rid = 0; rid < 3; ++rid)
        rig.log->wait_for_count("at_stop:" + std::to_string(rid), 1, 5000ms);
    std::this_thread::sleep_for(250ms);  // let them join the waiting queue
    assert_completes([stop] { stop->bus_arrives(); }, 5000ms,
                     "a bus serving 3 waiting riders");
    for (auto& probe : probes)
        probe.assert_completed(5000ms, "a waiting rider's boarding");
    KOAN_ASSERT_MSG(departs(*rig.log) == std::vector<int>{3},
                    "one bus, three riders: expected depart(3); log " +
                        rig.log->joined());
    KOAN_ASSERT_MSG(boarders(*rig.log) ==
                        std::vector<std::string>({"0", "1", "2"}),
                    "all three waiting riders (and nobody else) board; log " +
                        rig.log->joined());
}

KOAN_TEST(overflow_waits_for_next_bus) {
    auto rig = make_rig(false);
    auto stop = rig.stop;
    std::vector<Probe> probes;
    for (int rid = 0; rid < 8; ++rid) probes.push_back(spawn_rider(rig, rid));
    for (int rid = 0; rid < 8; ++rid)
        rig.log->wait_for_count("at_stop:" + std::to_string(rid), 1, 5000ms);
    std::this_thread::sleep_for(250ms);  // let them join the waiting queue
    assert_completes([stop] { stop->bus_arrives(); }, 5000ms,
                     "the first bus (5 of 8 riders fit)");
    KOAN_ASSERT_MSG(departs(*rig.log) == std::vector<int>{kCapacity},
                    "8 waiting, capacity 5: the first bus departs with 5; "
                    "log " + rig.log->joined());
    eventually([&] { return finished(probes) == kCapacity; }, 5000ms,
               "the 5 boarded riders should be done");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(finished(probes) == kCapacity,
                    "more riders than the bus's capacity got aboard");
    assert_completes([stop] { stop->bus_arrives(); }, 5000ms,
                     "the second bus (the 3 left behind)");
    for (auto& probe : probes)
        probe.assert_completed(5000ms, "every rider after two buses");
    KOAN_ASSERT_MSG(departs(*rig.log) == std::vector<int>({5, 3}),
                    "expected depart(5) then depart(3); log " +
                        rig.log->joined());
    KOAN_ASSERT_MSG(
        boarders(*rig.log) ==
            std::vector<std::string>({"0", "1", "2", "3", "4", "5", "6", "7"}),
        "every rider boards exactly once; log " + rig.log->joined());
}

// Riders who walk up mid-boarding take the NEXT bus, never this one.
KOAN_TEST(late_arrivals_wait) {
    for (int trial = 0; trial < 5; ++trial) {
        auto rig = make_rig(true);
        auto stop = rig.stop;
        auto log = rig.log;
        std::string t = "trial " + std::to_string(trial) + ": ";
        std::vector<Probe> early;
        for (int rid : {1, 2}) early.push_back(spawn_rider(rig, rid));
        for (int rid : {1, 2})
            log->wait_for_count("at_stop:" + std::to_string(rid), 1, 5000ms);
        std::this_thread::sleep_for(200ms);  // let both join the queue
        auto bus = spawn_probe([stop] { stop->bus_arrives(); });
        eventually(
            [&] {
                for (const auto& e : log->events())
                    if (e.rfind("board_begin:", 0) == 0) return true;
                return false;
            },
            5000ms, t + "boarding should have begun");
        // Boarding is now frozen mid-step; two more riders walk up.
        std::vector<Probe> late;
        for (int rid : {3, 4}) late.push_back(spawn_rider(rig, rid));
        for (int rid : {3, 4})
            log->wait_for_count("at_stop:" + std::to_string(rid), 1, 5000ms);
        // Give them every chance to (wrongly) sneak aboard.
        std::this_thread::sleep_for(200ms);
        rig.gate->open();
        bus.assert_completed(5000ms, t + "the bus's departure");
        KOAN_ASSERT_MSG(departs(*log) == std::vector<int>{2},
                        t + "only the 2 riders present at arrival board this "
                            "bus; log " + log->joined());
        KOAN_ASSERT_MSG(boarders(*log) == std::vector<std::string>({"1", "2"}),
                        t + "a mid-boarding walk-up boarded the current bus; "
                            "log " + log->joined());
        for (auto& probe : early)
            probe.assert_completed(5000ms, t + "an early rider's boarding");
        std::this_thread::sleep_for(200ms);
        KOAN_ASSERT_MSG(finished(late) == 0,
                        t + "a late rider finished without a second bus");
        assert_completes([stop] { stop->bus_arrives(); }, 5000ms,
                         t + "the next bus (for the late riders)");
        for (auto& probe : late)
            probe.assert_completed(5000ms,
                                   t + "a late rider on the next bus");
        KOAN_ASSERT_MSG(departs(*log) == std::vector<int>({2, 2}),
                        t + "each bus takes its own pair; log " +
                            log->joined());
        KOAN_ASSERT(boarders(*log) ==
                    std::vector<std::string>({"1", "2", "3", "4"}));
    }
}

KOAN_TEST(stress) {
    auto rig = make_rig(false);
    auto stop = rig.stop;
    auto log = rig.log;
    ThreadRunner runner;
    for (int rid = 0; rid < 30; ++rid) {
        runner.spawn(
            [stop, log, rid] {
                jitter(5);
                log->record("at_stop:" + std::to_string(rid));
                stop->rider(rid);
            },
            "r" + std::to_string(rid));
    }
    auto deadline = std::chrono::steady_clock::now() + 15s;
    while (boarders(*log).size() < 30) {
        KOAN_ASSERT_MSG(std::chrono::steady_clock::now() < deadline,
                        "the fleet never served all 30 riders; log has " +
                            std::to_string(boarders(*log).size()) +
                            " boardings");
        assert_completes([stop] { stop->bus_arrives(); }, 5000ms,
                         "a bus visit");
        jitter(3);
    }
    runner.join_all(5000ms);
    auto ds = departs(*log);
    int total = 0;
    for (int n : ds) {
        KOAN_ASSERT_MSG(n <= kCapacity, "some bus departed over capacity");
        total += n;
    }
    KOAN_ASSERT_MSG(total == 30,
                    "depart() counts must add up to the 30 riders served");
    std::vector<std::string> expect;
    for (int rid = 0; rid < 30; ++rid) expect.push_back(std::to_string(rid));
    std::sort(expect.begin(), expect.end());
    KOAN_ASSERT_MSG(boarders(*log) == expect,
                    "every rider boards exactly once");
}
