#include "koan_test.hpp"
#include "room_party.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

constexpr int kThreshold = 5;

// Per-student gates: party(sid) blocks until the test opens sid's gate
// (10 s failsafe so no thread is ever stranded).
class Gates {
  public:
    void open(int sid) {
        {
            std::lock_guard lock(mutex_);
            open_.insert(sid);
        }
        cond_.notify_all();
    }
    void await(int sid) {
        std::unique_lock lock(mutex_);
        cond_.wait_for(lock, 10s, [&] { return open_.count(sid) > 0; });
    }

  private:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::set<int> open_;
};

struct Rig {
    std::shared_ptr<Room> room;
    std::shared_ptr<EventLog> log;
    std::shared_ptr<Gates> gates;
};

Rig make_rig(bool hold) {
    auto log = std::make_shared<EventLog>();
    auto gates = std::make_shared<Gates>();
    auto room = std::make_shared<Room>(
        kThreshold, [log] { log->record("search"); },
        [log] { log->record("breakup"); },
        [log, gates, hold](int sid) {
            log->record("party:" + std::to_string(sid));
            if (hold) gates->await(sid);
        });
    return {room, log, gates};
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

Probe spawn_student(const Rig& rig, int sid) {
    auto room = rig.room;
    return spawn_probe([room, sid] { room->student_visit(sid); });
}

int finished(const std::vector<Probe>& probes, int upto) {
    int n = 0;
    for (int i = 0; i < upto; ++i)
        if (probes[static_cast<std::size_t>(i)].done->load()) ++n;
    return n;
}

// Wait until every student in sids is inside (its party event logged),
// failing fast if any student thread threw.
void wait_partying(const std::vector<Probe>& students,
                   const std::shared_ptr<EventLog>& log,
                   const std::vector<int>& sids) {
    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline) {
        for (const auto& p : students)
            if (p.done->load() && !p.error->empty())
                KOAN_FAIL("a student raised: " + *p.error);
        bool all = true;
        for (int sid : sids)
            if (log->count("party:" + std::to_string(sid)) == 0) all = false;
        if (all) return;
        std::this_thread::sleep_for(5ms);
    }
    KOAN_FAIL("students never all got into the room; log " + log->joined());
}

}  // namespace

KOAN_TEST(dean_searches_empty_room) {
    auto rig = make_rig(false);
    auto room = rig.room;
    assert_completes([room] { room->dean_visit(); }, 2000ms,
                     "dean_visit on an empty room (search-and-go)");
    KOAN_ASSERT_MSG(rig.log->count("search") == 1,
                    "search() should have run once; log " + rig.log->joined());
    KOAN_ASSERT_MSG(rig.log->count("breakup") == 0,
                    "there was no party to break up");
}

KOAN_TEST(dean_waits_on_small_party) {
    auto rig = make_rig(true);
    auto room = rig.room;
    std::vector<Probe> students;
    for (int sid : {0, 1}) students.push_back(spawn_student(rig, sid));
    wait_partying(students, rig.log, {0, 1});
    auto dean = assert_blocks(
        [room] { room->dean_visit(); }, 300ms,
        "dean_visit (2 students: too many to search, too few to break up)");
    KOAN_ASSERT_MSG(
        rig.log->count("search") == 0 && rig.log->count("breakup") == 0,
        "the dean acted on a room he may not enter; log " + rig.log->joined());
    rig.gates->open(0);
    rig.gates->open(1);
    dean.assert_completed(5000ms,
                          "the waiting dean once the room empties (search)");
    KOAN_ASSERT_MSG(rig.log->count("search") == 1,
                    "the dean entered an empty room: that's a search; log " +
                        rig.log->joined());
    KOAN_ASSERT_MSG(rig.log->count("breakup") == 0,
                    "there was never a big enough party to break up");
}

KOAN_TEST(big_party_gets_broken_up) {
    auto rig = make_rig(true);
    auto room = rig.room;
    auto log = rig.log;
    std::vector<Probe> students;
    std::vector<int> sids{0, 1, 2, 3, 4, 5};
    for (int sid : sids) students.push_back(spawn_student(rig, sid));
    wait_partying(students, log, sids);
    auto dean = spawn_probe([room] { room->dean_visit(); });
    eventually([&] { return log->count("breakup") == 1; }, 5000ms,
               "6 partiers with threshold 5: the dean must walk in and "
               "break it up");
    KOAN_ASSERT_MSG(log->count("search") == 0,
                    "the dean searched a crowded room");
    // Two students try to slip in while the dean is inside.
    std::vector<Probe> late;
    for (int sid : {10, 11}) late.push_back(spawn_student(rig, sid));
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("party:10") == 0 && log->count("party:11") == 0,
                    "a student entered while the dean was in the room; log " +
                        log->joined());
    // The partiers file out; the dean must stay until the last is gone.
    for (int sid = 0; sid < 5; ++sid) rig.gates->open(sid);
    eventually([&] { return finished(students, 5) == 5; }, 5000ms,
               "students must be able to leave while the dean is in the room");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(!dean.done->load(),
                    "the dean left before the room was empty");
    KOAN_ASSERT_MSG(log->count("party:10") == 0 && log->count("party:11") == 0,
                    "a student slipped in mid-breakup; log " + log->joined());
    rig.gates->open(5);
    dean.assert_completed(5000ms, "the dean once the last student leaves");
    // Only now may the two latecomers get in.
    rig.gates->open(10);
    rig.gates->open(11);
    for (auto& probe : late)
        probe.assert_completed(5000ms,
                               "a latecomer's visit once the dean is gone");
    KOAN_ASSERT(log->count("party:10") == 1 && log->count("party:11") == 1);
    KOAN_ASSERT(log->count("breakup") == 1 && log->count("search") == 0);
}

KOAN_TEST(students_may_leave_while_dean_inside) {
    auto rig = make_rig(true);
    auto room = rig.room;
    auto log = rig.log;
    std::vector<Probe> students;
    std::vector<int> sids{0, 1, 2, 3, 4, 5};
    for (int sid : sids) students.push_back(spawn_student(rig, sid));
    wait_partying(students, log, sids);
    auto dean = spawn_probe([room] { room->dean_visit(); });
    eventually([&] { return log->count("breakup") == 1; }, 5000ms,
               "6 partiers with threshold 5: the dean must walk in and "
               "break it up");
    for (int sid = 0; sid < 5; ++sid) rig.gates->open(sid);
    eventually([&] { return finished(students, 5) == 5; }, 5000ms,
               "leaving must not be barred by the dean's presence");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(!dean.done->load(),
                    "the dean left with a student still inside");
    rig.gates->open(5);
    dean.assert_completed(5000ms,
                          "the dean once the last student is out");
}

KOAN_TEST(growth_while_dean_waits) {
    auto rig = make_rig(true);
    auto room = rig.room;
    auto log = rig.log;
    std::vector<Probe> students;
    for (int sid : {0, 1}) students.push_back(spawn_student(rig, sid));
    wait_partying(students, log, {0, 1});
    auto dean = assert_blocks(
        [room] { room->dean_visit(); }, 300ms,
        "dean_visit (2 students: too many to search, too few to break up)");
    KOAN_ASSERT_MSG(
        log->count("search") == 0 && log->count("breakup") == 0,
        "the dean acted on a room he may not enter; log " + log->joined());
    // The party grows while the dean waits — he is not in the room, so
    // students walk right past him.
    for (int sid : {2, 3, 4, 5}) students.push_back(spawn_student(rig, sid));
    eventually([&] { return log->count("breakup") == 1; }, 5000ms,
               "the party outgrew the threshold — the waiting dean must "
               "storm in");
    KOAN_ASSERT_MSG(log->count("search") == 0,
                    "the dean searched a room full of students");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(!dean.done->load(),
                    "the dean left with students still inside");
    for (int sid = 0; sid < 6; ++sid) rig.gates->open(sid);
    dean.assert_completed(5000ms, "the dean once everyone has left");
    for (auto& probe : students)
        probe.assert_completed(5000ms, "every student's visit");
    KOAN_ASSERT(log->count("breakup") == 1 && log->count("search") == 0);
}
