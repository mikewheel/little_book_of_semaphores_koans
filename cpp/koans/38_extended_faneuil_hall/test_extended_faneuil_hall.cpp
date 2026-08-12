#include "koan_test.hpp"
#include "extended_faneuil_hall.hpp"

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

namespace {

// A latch the tests hold shut to freeze a hook mid-ceremony. wait_open has
// a generous cap so an orphaned hook can never outlive the test by much.
class Gate {
  public:
    void open() {
        {
            std::lock_guard<std::mutex> lock(m_);
            open_ = true;
        }
        cv_.notify_all();
    }
    void wait_open() {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait_for(lock, std::chrono::seconds(20), [&] { return open_; });
    }

  private:
    std::mutex m_;
    std::condition_variable cv_;
    bool open_ = false;
};

using GateMap = std::map<std::string, std::shared_ptr<Gate>>;

std::shared_ptr<GateMap> gate_map(std::initializer_list<std::string> labels) {
    auto gates = std::make_shared<GateMap>();
    for (const auto& label : labels) (*gates)[label] = std::make_shared<Gate>();
    return gates;
}

// Opens every gate at scope exit, so a failing assertion can never strand
// a gated hook (and the thread inside it) behind a closed gate.
struct OpenAllOnExit {
    std::shared_ptr<GateMap> gates;
    ~OpenAllOnExit() {
        if (gates)
            for (auto& [label, gate] : *gates) gate->open();
    }
};

// Hooks record "<label>" on completion; a gated hook records
// "<label>:pending" first, then blocks until its gate opens.
std::shared_ptr<ExtendedFaneuilHall> make_hall(std::shared_ptr<EventLog> log,
                                               std::shared_ptr<GateMap> gates) {
    auto fire = [log, gates](const std::string& label) {
        if (gates) {
            auto it = gates->find(label);
            if (it != gates->end()) {
                log->record(label + ":pending");
                it->second->wait_open();
            }
        }
        log->record(label);
    };
    FaneuilHooks h;
    h.enter = [fire](const std::string& who) { fire("enter:" + who); };
    h.check_in = [fire](int iid) { fire("check_in:" + std::to_string(iid)); };
    h.sit_down = [fire](int iid) { fire("sit_down:" + std::to_string(iid)); };
    h.swear = [fire](int iid) { fire("swear:" + std::to_string(iid)); };
    h.get_certificate = [fire](int iid) {
        fire("certificate:" + std::to_string(iid));
    };
    h.confirm = [fire] { fire("confirm"); };
    h.spectate = [fire](int sid) { fire("spectate:" + std::to_string(sid)); };
    h.leave = [fire](const std::string& who) { fire("leave:" + who); };
    return std::make_shared<ExtendedFaneuilHall>(std::move(h));
}

// Fails fast (with the NotImplemented message) while the starter is empty:
// a spectator strolling through a judge-free hall must always sail through.
void starter_tripwire() {
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    assert_completes([hall] { hall->spectator(0); }, 5000ms,
                     "a spectator strolling through an empty hall");
}

long index_of(const std::vector<std::string>& events, const std::string& label,
              int occurrence = 0) {
    int seen = 0;
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (events[i] == label && seen++ == occurrence)
            return static_cast<long>(i);
    }
    KOAN_FAIL("event '" + label + "' (occurrence " +
              std::to_string(occurrence) + ") not found");
}

// THE extended-rule scenario: sworn immigrants linger (held mid-certificate,
// holding no internal locks); the judge has left; her next visit's enter
// must wait for the last of them to go.
void run_reentry_trial() {
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"certificate:1", "certificate:2", "certificate:3"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    for (int i : {1, 2, 3})
        runner.spawn([hall, i] { hall->immigrant(i); },
                     "imm" + std::to_string(i));
    for (int i : {1, 2, 3})
        log->wait_for_count("check_in:" + std::to_string(i), 1, 5000ms);
    runner.spawn([hall] { hall->judge_visit(); }, "judge1");
    log->wait_for_count("leave:judge", 1, 5000ms);  // the judge walked out
    for (int i : {1, 2, 3})  // ...but they all linger
        log->wait_for_count("certificate:" + std::to_string(i) + ":pending", 1,
                            5000ms);
    runner.spawn([hall] { hall->judge_visit(); }, "judge2");  // back already
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("enter:judge") == 1,
                    "the judge re-entered while sworn immigrants were still "
                    "inside; log was " + log->joined());
    for (int i : {1, 2}) {  // let them go one at a time
        auto id = std::to_string(i);
        (*gates)["certificate:" + id]->open();
        log->wait_for_count("leave:immigrant:" + id, 1, 5000ms);
        std::this_thread::sleep_for(150ms);
        KOAN_ASSERT_MSG(log->count("enter:judge") == 1,
                        "the judge re-entered with sworn immigrants still "
                        "inside (after " + id + " of 3 had left); log was " +
                            log->joined());
    }
    (*gates)["certificate:3"]->open();  // the last one out
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("enter:judge"), std::size_t{2});
    auto events = log->events();
    long second_entry = index_of(events, "enter:judge", 1);
    for (int i : {1, 2, 3})
        KOAN_ASSERT_MSG(
            index_of(events, "leave:immigrant:" + std::to_string(i)) <
                second_entry,
            "the judge's second enter fired before immigrant " +
                std::to_string(i) + " left");
}

}  // namespace

KOAN_TEST(ceremony_happy_path) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    ThreadRunner runner;
    for (int i : {1, 2, 3})
        runner.spawn([hall, i] { hall->immigrant(i); },
                     "imm" + std::to_string(i));
    for (int i : {1, 2, 3})
        log->wait_for_count("check_in:" + std::to_string(i), 1, 5000ms);
    runner.spawn([hall] { hall->judge_visit(); }, "judge");
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("confirm"), std::size_t{1});
    for (int i : {1, 2, 3}) {
        auto id = std::to_string(i);
        log->assert_before("check_in:" + id, "confirm");
        log->assert_before("confirm", "certificate:" + id);
        log->assert_before("certificate:" + id, "leave:immigrant:" + id);
        log->assert_before("leave:judge", "leave:immigrant:" + id);
    }
}

KOAN_TEST(judge_waits_for_checkins) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"check_in:2"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    for (int i : {1, 3})
        runner.spawn([hall, i] { hall->immigrant(i); },
                     "imm" + std::to_string(i));
    log->wait_for_count("check_in:1", 1, 5000ms);
    log->wait_for_count("check_in:3", 1, 5000ms);
    runner.spawn([hall] { hall->immigrant(2); }, "imm2");  // the slow one
    log->wait_for_count("check_in:2:pending", 1, 5000ms);  // 2 is mid-check-in
    runner.spawn([hall] { hall->judge_visit(); }, "judge");
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("confirm") == 0,
                    "the judge confirmed before every immigrant who entered "
                    "had checked in; log was " + log->joined());
    (*gates)["check_in:2"]->open();
    runner.join_all(10000ms);
    log->assert_before("check_in:2", "confirm");
    KOAN_ASSERT_EQ(log->count("confirm"), std::size_t{1});
}

KOAN_TEST(door_locked_while_judge_inside) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"confirm"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    runner.spawn([hall] { hall->immigrant(1); }, "imm1");
    log->wait_for_count("check_in:1", 1, 5000ms);
    runner.spawn([hall] { hall->judge_visit(); }, "judge");
    log->wait_for_count("confirm:pending", 1, 5000ms);  // judge mid-ceremony
    runner.spawn([hall] { hall->immigrant(9); }, "imm9");
    runner.spawn([hall] { hall->spectator(7); }, "spec7");
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("enter:immigrant:9") == 0,
                    "an immigrant entered while the judge was in the building");
    KOAN_ASSERT_MSG(log->count("enter:spectator:7") == 0,
                    "a spectator entered while the judge was in the building");
    (*gates)["confirm"]->open();
    // Once the ceremony wraps up, both walk in.
    log->wait_for_count("enter:immigrant:9", 1, 5000ms);
    log->wait_for_count("enter:spectator:7", 1, 5000ms);
    log->assert_before("leave:judge", "enter:immigrant:9");
    log->assert_before("leave:judge", "enter:spectator:7");
    // A second visit swears in the latecomer so everyone can finish.
    log->wait_for_count("check_in:9", 1, 5000ms);
    runner.spawn([hall] { hall->judge_visit(); }, "judge2");
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("confirm"), std::size_t{2});
}

KOAN_TEST(judge_cannot_reenter_until_sworn_leave) {
    starter_tripwire();
    for (int trial = 0; trial < 5; ++trial) run_reentry_trial();
}

KOAN_TEST(second_ceremony_clean_counts) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    ThreadRunner runner;
    std::vector<std::vector<int>> batches = {{1, 2}, {3, 4, 5}};
    int spectator_id = 51;
    for (const auto& batch : batches) {
        for (int i : batch)
            runner.spawn(
                [hall, i] {
                    jitter();
                    hall->immigrant(i);
                },
                "imm" + std::to_string(i));
        int sid = spectator_id++;
        runner.spawn(
            [hall, sid] {
                jitter();
                hall->spectator(sid);
            },
            "spec" + std::to_string(sid));
        for (int i : batch)
            log->wait_for_count("check_in:" + std::to_string(i), 1, 5000ms);
        runner.spawn([hall] { hall->judge_visit(); }, "judge");
        runner.join_all(10000ms);
    }
    KOAN_ASSERT_EQ(log->count("confirm"), std::size_t{2});
    KOAN_ASSERT_EQ(log->count("enter:judge"), std::size_t{2});
    KOAN_ASSERT_EQ(log->count("leave:judge"), std::size_t{2});
    for (int i : {1, 2, 3, 4, 5}) {
        auto id = std::to_string(i);
        KOAN_ASSERT_EQ(log->count("check_in:" + id), std::size_t{1});
        KOAN_ASSERT_EQ(log->count("certificate:" + id), std::size_t{1});
        KOAN_ASSERT_EQ(log->count("leave:immigrant:" + id), std::size_t{1});
    }
    for (int s : {51, 52})
        KOAN_ASSERT_EQ(log->count("leave:spectator:" + std::to_string(s)),
                       std::size_t{1});
}
