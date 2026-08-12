#include "koan_test.hpp"
#include "santa.hpp"

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

// Santa is a detached daemon parked on a semaphore inside the NorthPole,
// so a NorthPole can never be safely destroyed. Tests leak it on purpose.

namespace {

// A gate several test threads can block on until the test opens it.
struct Gate {
    std::mutex mutex;
    std::condition_variable cond;
    bool open = false;

    bool wait(std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex);
        return cond.wait_for(lock, timeout, [&] { return open; });
    }
    void release() {
        {
            std::lock_guard lock(mutex);
            open = true;
        }
        cond.notify_all();
    }
};

// Records every hook call; get_help can be gated by the test.
struct Recorder {
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();
    std::shared_ptr<Gate> gate;  // null = get_help returns immediately

    NorthPoleHooks hooks() {
        auto log_ = log;
        auto gate_ = gate;
        NorthPoleHooks h;
        h.prepare_sleigh = [log_] { log_->record("sleigh"); };
        h.get_hitched = [log_](int rid) {
            log_->record("hitched" + std::to_string(rid));
        };
        h.help_elves = [log_] { log_->record("help"); };
        h.get_help = [log_, gate_](int eid) {
            if (gate_) gate_->wait(10000ms);
            log_->record("helped" + std::to_string(eid));
        };
        return h;
    }
};

std::size_t count_prefix(const EventLog& log, const std::string& prefix) {
    std::size_t n = 0;
    for (const auto& e : log.events())
        if (e.rfind(prefix, 0) == 0) ++n;
    return n;
}

}  // namespace

KOAN_TEST(reindeer_flight) {
    Recorder rec;
    auto* pole = new NorthPole(rec.hooks());  // leaked on purpose
    pole->start_santa();
    auto log = rec.log;
    ThreadRunner runner;

    // Eight reindeer home: Santa must keep sleeping — no sleigh yet.
    for (int rid = 0; rid < 8; ++rid) {
        runner.spawn([pole, rid] { pole->reindeer_arrives(rid); },
                     "reindeer-" + std::to_string(rid));
        jitter();
    }
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("sleigh") == 0,
                    "Santa prepped the sleigh before the last reindeer was "
                    "home");

    // The ninth springs him into action: one sleigh, nine hitchings.
    runner.spawn([pole] { pole->reindeer_arrives(8); }, "reindeer-8");
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("sleigh"), static_cast<std::size_t>(1));
    for (int rid = 0; rid < 9; ++rid)
        KOAN_ASSERT_EQ(log->count("hitched" + std::to_string(rid)),
                       static_cast<std::size_t>(1));
    log->assert_before("sleigh", "hitched0");  // prep first, then hitch
}

KOAN_TEST(elves_in_batches_of_three) {
    Recorder rec;
    auto* pole = new NorthPole(rec.hooks());  // leaked on purpose
    pole->start_santa();
    auto log = rec.log;
    ThreadRunner runner;
    for (int eid = 0; eid < 3; ++eid) {
        runner.spawn([pole, eid] { pole->elf_needs_help(eid); },
                     "elf-" + std::to_string(eid));
        jitter();
    }
    runner.join_all(10000ms);
    KOAN_ASSERT_MSG(log->count("help") == 1, "one help_elves per group of 3");
    for (int eid = 0; eid < 3; ++eid)
        KOAN_ASSERT_EQ(log->count("helped" + std::to_string(eid)),
                       static_cast<std::size_t>(1));
    log->assert_before("help", "helped0");  // Santa helps, then they get it
}

KOAN_TEST(fourth_elf_waits_for_a_new_group) {
    Recorder rec;
    rec.gate = std::make_shared<Gate>();
    auto* pole = new NorthPole(rec.hooks());  // leaked on purpose
    pole->start_santa();
    auto log = rec.log;
    auto gate = rec.gate;
    ThreadRunner runner;

    // A full group of three walks in; their get_help hangs on the gate.
    for (int eid = 0; eid < 3; ++eid) {
        runner.spawn([pole, eid] { pole->elf_needs_help(eid); },
                     "elf-" + std::to_string(eid));
    }
    log->wait_for_count("help", 1, 5000ms);

    // Three more elves arrive mid-help: none may sneak into the group in
    // progress — they must wait at the door and form the NEXT group.
    for (int eid = 3; eid < 6; ++eid) {
        runner.spawn([pole, eid] { pole->elf_needs_help(eid); },
                     "elf-" + std::to_string(eid));
    }
    std::this_thread::sleep_for(300ms);
    for (int eid = 3; eid < 6; ++eid)
        KOAN_ASSERT_MSG(log->count("helped" + std::to_string(eid)) == 0,
                        "an elf joined a group that was already being helped");
    KOAN_ASSERT_EQ(log->count("help"), static_cast<std::size_t>(1));

    gate->release();  // first group finishes; the second forms and is helped
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("help"), static_cast<std::size_t>(2));
    for (int eid = 0; eid < 6; ++eid)
        KOAN_ASSERT_EQ(log->count("helped" + std::to_string(eid)),
                       static_cast<std::size_t>(1));

    // And a lone straggler keeps waiting until two buddies show up.
    runner.spawn([pole] { pole->elf_needs_help(6); }, "elf-6");
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("helped6") == 0,
                    "an elf was helped without a full group");
    runner.spawn([pole] { pole->elf_needs_help(7); }, "elf-7");
    runner.spawn([pole] { pole->elf_needs_help(8); }, "elf-8");
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("help"), static_cast<std::size_t>(3));
    KOAN_ASSERT_EQ(log->count("helped6"), static_cast<std::size_t>(1));
}

KOAN_TEST(multiple_cycles) {
    Recorder rec;
    auto* pole = new NorthPole(rec.hooks());  // leaked on purpose
    pole->start_santa();
    auto log = rec.log;
    ThreadRunner runner;
    for (int eid = 0; eid < 12; ++eid) {  // four full elf groups
        runner.spawn([pole, eid] { pole->elf_needs_help(eid); },
                     "elf-" + std::to_string(eid));
        jitter();
    }
    for (int rid = 0; rid < 9; ++rid) {  // flight one
        runner.spawn([pole, rid] { pole->reindeer_arrives(rid); },
                     "reindeer-" + std::to_string(rid));
        jitter();
    }
    // The same nine reindeer fly every year: the next herd cannot start
    // arriving until this year's flight is fully hitched (see README).
    eventually([log] { return count_prefix(*log, "hitched") == 9; }, 10000ms,
               "the first flight never finished");
    for (int rid = 9; rid < 18; ++rid) {  // flight two
        runner.spawn([pole, rid] { pole->reindeer_arrives(rid); },
                     "reindeer-" + std::to_string(rid));
        jitter();
    }
    runner.join_all(15000ms);

    KOAN_ASSERT_MSG(log->count("sleigh") == 2,
                    "18 reindeer == exactly 2 flights");
    KOAN_ASSERT_MSG(log->count("help") == 4, "12 elves == exactly 4 groups");
    KOAN_ASSERT_EQ(count_prefix(*log, "hitched"), static_cast<std::size_t>(18));
    KOAN_ASSERT_EQ(count_prefix(*log, "helped"), static_cast<std::size_t>(12));
}

KOAN_TEST(batch_atomicity_stress) {
    // Nine elves at once — spawned as fast as possible so arrivals overlap
    // groups already in flight: exactly three groups, and the three elves
    // helped after each help_elves must be three distinct elves who never
    // appear in a later group.
    Recorder rec;
    auto* pole = new NorthPole(rec.hooks());  // leaked on purpose
    pole->start_santa();
    auto log = rec.log;
    ThreadRunner runner;
    for (int eid = 0; eid < 9; ++eid) {
        runner.spawn([pole, eid] { pole->elf_needs_help(eid); },
                     "elf-" + std::to_string(eid));
    }
    runner.join_all(15000ms);

    auto events = log->events();
    KOAN_ASSERT_MSG(log->count("help") == 3,
                    "9 elves == exactly 3 groups: " + log->joined());

    std::vector<std::set<std::string>> groups;
    for (const auto& e : events) {
        if (e == "help") {
            groups.emplace_back();
        } else if (e.rfind("helped", 0) == 0) {
            KOAN_ASSERT_MSG(!groups.empty(),
                            "get_help before any help_elves: " + log->joined());
            groups.back().insert(e.substr(6));
        }
    }
    std::set<std::string> seen;
    for (const auto& g : groups) {
        KOAN_ASSERT_MSG(g.size() == 3,
                        "each group must be exactly 3 elves: " + log->joined());
        for (const auto& eid : g) {
            KOAN_ASSERT_MSG(!seen.count(eid),
                            "an elf appears in two groups: " + log->joined());
            seen.insert(eid);
        }
    }
    KOAN_ASSERT_EQ(seen.size(), static_cast<std::size_t>(9));
}
