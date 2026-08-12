#include "koan_test.hpp"
#include "extended_dining_hall.hpp"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>

using namespace koans;

namespace {

// A latch the tests hold shut to keep a student at the table. wait_open has
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
// "<label>:pending" first, then blocks until its gate opens. A gated
// "dine:<sid>" therefore means "student <sid> is at the table eating".
std::shared_ptr<ExtendedDiningHall> make_hall(std::shared_ptr<EventLog> log,
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
    DiningHooks h;
    h.get_food = [fire](int sid) { fire("food:" + std::to_string(sid)); };
    h.dine = [fire](int sid) { fire("dine:" + std::to_string(sid)); };
    h.leave = [fire](int sid) { fire("leave:" + std::to_string(sid)); };
    return std::make_shared<ExtendedDiningHall>(std::move(h));
}

// Fails fast (with the NotImplemented message) while the starter is empty:
// a pair of students must always be able to dine together and go.
void starter_tripwire() {
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    ThreadRunner runner;
    runner.spawn([hall] { hall->student(-1); }, "trip1");
    runner.spawn([hall] { hall->student(-2); }, "trip2");
    runner.join_all(5000ms);
}

}  // namespace

KOAN_TEST(first_student_waits_to_dine) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    ThreadRunner runner;
    runner.spawn([hall] { hall->student(1); }, "s1");
    log->wait_for_count("food:1", 1, 5000ms);  // she has her tray...
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("dine:1") == 0,
                    "student 1 sat down to eat all alone; log was " +
                        log->joined());
    runner.spawn([hall] { hall->student(2); }, "s2");  // company arrives
    log->wait_for_count("dine:1", 1, 5000ms);  // they sit down together
    log->wait_for_count("dine:2", 1, 5000ms);
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("leave:1"), std::size_t{1});
    KOAN_ASSERT_EQ(log->count("leave:2"), std::size_t{1});
}

KOAN_TEST(joins_existing_diner_immediately) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"dine:1", "dine:2", "dine:3"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    runner.spawn([hall] { hall->student(1); }, "s1");
    runner.spawn([hall] { hall->student(2); }, "s2");
    log->wait_for_count("dine:1:pending", 1, 5000ms);  // the pair is eating
    log->wait_for_count("dine:2:pending", 1, 5000ms);
    runner.spawn([hall] { hall->student(3); }, "s3");
    // Someone is already dining, so student 3 sits down promptly.
    log->wait_for_count("dine:3:pending", 1, 5000ms);
    for (auto& [label, gate] : *gates) gate->open();
    runner.join_all(10000ms);
    for (int i : {1, 2, 3})
        KOAN_ASSERT_EQ(log->count("leave:" + std::to_string(i)),
                       std::size_t{1});
}

KOAN_TEST(early_finisher_waits) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"dine:1", "dine:2"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    runner.spawn([hall] { hall->student(1); }, "s1");
    runner.spawn([hall] { hall->student(2); }, "s2");
    log->wait_for_count("dine:1:pending", 1, 5000ms);  // both at the table
    log->wait_for_count("dine:2:pending", 1, 5000ms);
    (*gates)["dine:1"]->open();  // student 1 finishes first
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("leave:1") == 0,
                    "student 1 walked out and stranded student 2 eating "
                    "alone; log was " + log->joined());
    (*gates)["dine:2"]->open();  // student 2 finishes: they go together
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("leave:1"), std::size_t{1});
    KOAN_ASSERT_EQ(log->count("leave:2"), std::size_t{1});
}

KOAN_TEST(newcomer_releases_waiter) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto gates = gate_map({"dine:1", "dine:2", "dine:3"});
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    runner.spawn([hall] { hall->student(1); }, "s1");
    runner.spawn([hall] { hall->student(2); }, "s2");
    log->wait_for_count("dine:1:pending", 1, 5000ms);
    log->wait_for_count("dine:2:pending", 1, 5000ms);
    (*gates)["dine:1"]->open();  // student 1 is done, student 2 eats on
    std::this_thread::sleep_for(100ms);  // let 1 get stuck politely
    runner.spawn([hall] { hall->student(3); }, "s3");  // newcomer joins
    log->wait_for_count("dine:3:pending", 1, 5000ms);
    // With 2 and 3 at the table, student 1 is free to go.
    log->wait_for_count("leave:1", 1, 5000ms);
    KOAN_ASSERT_EQ(log->count("leave:2"), std::size_t{0});
    KOAN_ASSERT_EQ(log->count("leave:3"), std::size_t{0});
    (*gates)["dine:2"]->open();  // now 2 finishes; 3 would be stranded
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(log->count("leave:2") == 0,
                    "student 2 walked out and stranded student 3 eating "
                    "alone; log was " + log->joined());
    (*gates)["dine:3"]->open();  // 3 finishes: 2 and 3 go together
    runner.join_all(10000ms);
    for (int i : {1, 2, 3})
        KOAN_ASSERT_EQ(log->count("leave:" + std::to_string(i)),
                       std::size_t{1});
}

KOAN_TEST(pairs_leave_together) {
    starter_tripwire();
    auto log = std::make_shared<EventLog>();
    auto hall = make_hall(log, nullptr);
    auto gate1 = std::make_shared<Gate>();
    auto gate2 = std::make_shared<Gate>();
    // Belt and braces: open both gates even if an assertion fires first.
    struct OpenOnExit {
        std::shared_ptr<Gate> a, b;
        ~OpenOnExit() {
            a->open();
            b->open();
        }
    } opener{gate1, gate2};
    ThreadRunner runner;
    runner.spawn([hall, gate1] { hall->student(1, [gate1] { gate1->wait_open(); }); },
                 "s1");
    runner.spawn([hall, gate2] { hall->student(2, [gate2] { gate2->wait_open(); }); },
                 "s2");
    log->wait_for_count("dine:1", 1, 5000ms);  // seated together
    log->wait_for_count("dine:2", 1, 5000ms);
    gate1->open();  // both finish (near-)simultaneously
    gate2->open();
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(log->count("leave:1"), std::size_t{1});
    KOAN_ASSERT_EQ(log->count("leave:2"), std::size_t{1});
}

KOAN_TEST(full_lifecycle_stress) {
    starter_tripwire();
    constexpr int n = 12;
    auto log = std::make_shared<EventLog>();
    auto gates = std::make_shared<GateMap>();
    for (int i = 0; i < n; ++i)
        (*gates)["dine:" + std::to_string(i)] = std::make_shared<Gate>();
    OpenAllOnExit guard{gates};
    auto hall = make_hall(log, gates);
    ThreadRunner runner;
    for (int i = 0; i < n; ++i) {  // staggered arrivals
        runner.spawn([hall, i] { hall->student(i); }, "s" + std::to_string(i));
        jitter(8);
    }
    for (int i = 0; i < n; ++i)  // everyone ends up seated
        log->wait_for_count("dine:" + std::to_string(i) + ":pending", 1,
                            5000ms);
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(order.begin(), order.end(), rng);
    for (int i : order) {  // they finish eating in random order
        (*gates)["dine:" + std::to_string(i)]->open();
        jitter(8);
    }
    runner.join_all(10000ms);
    for (int i = 0; i < n; ++i) {
        auto id = std::to_string(i);
        KOAN_ASSERT_EQ(log->count("food:" + id), std::size_t{1});
        KOAN_ASSERT_EQ(log->count("dine:" + id), std::size_t{1});
        KOAN_ASSERT_EQ(log->count("leave:" + id), std::size_t{1});
    }
}
