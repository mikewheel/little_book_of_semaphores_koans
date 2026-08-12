#include "koan_test.hpp"
#include "signaling.hpp"

#include <memory>
#include <thread>

using namespace koans;

KOAN_TEST(b1_runs_after_a1_even_when_b_starts_first) {
    for (int trial = 0; trial < 30; ++trial) {
        Signaling sig;
        EventLog log;
        ThreadRunner runner;
        runner.spawn([&] { sig.run_b([&] { log.record("b1"); }); }, "B");
        runner.spawn(
            [&] {
                std::this_thread::sleep_for(2ms);
                sig.run_a([&] { log.record("a1"); });
            },
            "A");
        runner.join_all(5000ms);
        KOAN_ASSERT_MSG(log.events() == std::vector<std::string>({"a1", "b1"}),
                        "b1 must come after a1; log was " + log.joined());
    }
}

KOAN_TEST(b_blocks_until_a_signals) {
    auto sig = std::make_shared<Signaling>();
    auto log = std::make_shared<EventLog>();
    auto probe = assert_blocks(
        [sig, log] { sig->run_b([log] { log->record("b1"); }); }, 300ms,
        "run_b (while A has not yet run)");
    sig->run_a([log] { log->record("a1"); });
    probe.assert_completed(5000ms, "run_b after A signaled");
    log->assert_before("a1", "b1");
}

KOAN_TEST(a_never_waits_for_b) {
    auto sig = std::make_shared<Signaling>();
    assert_completes([sig] { sig->run_a([] {}); }, 2000ms,
                     "run_a (B never shows up)");
}

KOAN_TEST(signal_persists_if_a_finishes_first) {
    auto sig = std::make_shared<Signaling>();
    auto log = std::make_shared<EventLog>();
    sig->run_a([log] { log->record("a1"); });
    std::this_thread::sleep_for(50ms);  // the signal must not evaporate
    assert_completes([sig, log] { sig->run_b([log] { log->record("b1"); }); },
                     2000ms, "run_b after A already finished");
    KOAN_ASSERT(log->events() == std::vector<std::string>({"a1", "b1"}));
}

KOAN_TEST(stress_random_interleavings) {
    for (int trial = 0; trial < 100; ++trial) {
        Signaling sig;
        EventLog log;
        ThreadRunner runner;
        runner.spawn([&] {
            jitter();
            sig.run_b([&] { log.record("b1"); });
        });
        runner.spawn([&] {
            jitter();
            sig.run_a([&] { log.record("a1"); });
        });
        runner.join_all(5000ms);
        KOAN_ASSERT_MSG(log.events() == std::vector<std::string>({"a1", "b1"}),
                        "log was " + log.joined());
    }
}
