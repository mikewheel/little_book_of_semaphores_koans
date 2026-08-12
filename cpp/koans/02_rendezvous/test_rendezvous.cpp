#include "koan_test.hpp"
#include "rendezvous.hpp"

#include <memory>
#include <set>
#include <thread>

using namespace koans;

namespace {

EventLog run_pair(std::chrono::milliseconds a_delay = 0ms,
                  std::chrono::milliseconds b_delay = 0ms) {
    Rendezvous rv;
    EventLog log;
    ThreadRunner runner;
    runner.spawn(
        [&] {
            std::this_thread::sleep_for(a_delay);
            jitter();
            rv.run_a([&] { log.record("a1"); }, [&] { log.record("a2"); });
        },
        "A");
    runner.spawn(
        [&] {
            std::this_thread::sleep_for(b_delay);
            jitter();
            rv.run_b([&] { log.record("b1"); }, [&] { log.record("b2"); });
        },
        "B");
    runner.join_all(5000ms);
    return log;
}

void assert_rendezvous_order(const EventLog& log) {
    log.assert_before("a1", "b2");
    log.assert_before("b1", "a2");
}

}  // namespace

KOAN_TEST(meeting_when_a_is_slow) {
    for (int i = 0; i < 10; ++i) assert_rendezvous_order(run_pair(5ms, 0ms));
}

KOAN_TEST(meeting_when_b_is_slow) {
    for (int i = 0; i < 10; ++i) assert_rendezvous_order(run_pair(0ms, 5ms));
}

KOAN_TEST(first_arrival_blocks_until_partner_shows_up) {
    auto rv = std::make_shared<Rendezvous>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [rv, log] {
            rv->run_a([log] { log->record("a1"); }, [log] { log->record("a2"); });
        },
        300ms, "run_a (B has not arrived)");
    std::thread([rv, log] {
        rv->run_b([log] { log->record("b1"); }, [log] { log->record("b2"); });
    }).detach();
    probe.assert_completed(5000ms, "run_a once B arrives");
    log->wait_for_count("b2", 1, 5000ms);
    assert_rendezvous_order(*log);
}

KOAN_TEST(symmetric_first_arrival_b_blocks_too) {
    auto rv = std::make_shared<Rendezvous>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [rv, log] {
            rv->run_b([log] { log->record("b1"); }, [log] { log->record("b2"); });
        },
        300ms, "run_b (A has not arrived)");
    std::thread([rv, log] {
        rv->run_a([log] { log->record("a1"); }, [log] { log->record("a2"); });
    }).detach();
    probe.assert_completed(5000ms, "run_b once A arrives");
    log->wait_for_count("a2", 1, 5000ms);
    assert_rendezvous_order(*log);
}

// Either thread may finish its first statement first. A solution that
// serializes a1 before b1 (or vice versa) enforces too much.
KOAN_TEST(does_not_overconstrain_a1_b1_order) {
    std::set<std::string> firsts;
    for (int i = 0; i < 200 && firsts.size() < 2; ++i) {
        auto log = run_pair();
        assert_rendezvous_order(log);
        auto evs = log.events();
        for (const auto& e : evs) {
            if (e == "a1" || e == "b1") {
                firsts.insert(e);
                break;
            }
        }
    }
    KOAN_ASSERT_MSG(firsts.size() == 2,
                    "over 200 runs only one of a1/b1 ever came first — "
                    "the rendezvous is over-constrained");
}

KOAN_TEST(stress) {
    for (int i = 0; i < 100; ++i) assert_rendezvous_order(run_pair());
}
