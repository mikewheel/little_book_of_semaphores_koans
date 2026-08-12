#include "koan_test.hpp"
#include "extended_child_care.hpp"

#include <atomic>
#include <memory>
#include <thread>

using namespace koans;

namespace {

constexpr int kRatio = 3;

// Atomic occupancy snapshot (the probe category is transient).
std::map<std::string, int> snapshot(OverlapTracker& tracker) {
    auto snap = tracker.enter("probe");
    tracker.exit("probe");
    return snap;
}

void check_ratio(OverlapTracker& tracker, std::map<std::string, int> snap,
                 const std::string& where) {
    if (snap["children"] > kRatio * snap["adults"])
        tracker.violate(where + ": " + std::to_string(snap["children"]) +
                        " children with only " +
                        std::to_string(snap["adults"]) + " adult(s) inside");
}

}  // namespace

// ── Everything koan 33 demanded still holds ──────────────────────────────

// Stress: adults and children churn; the bound holds at every sample.
// Bookkeeping order avoids false alarms: adults are counted from just
// before adult_enter until just after adult_leave returns (over-count,
// which also matches the rule that a waiting leaver is still inside),
// children only between child_enter returning and child_leave being
// called (under-count). Real violations still show up.
KOAN_TEST(ratio_never_violated) {
    ExtendedChildCare cc(kRatio);
    OverlapTracker tracker;
    ThreadRunner runner;
    std::atomic<bool> children_done{false};
    std::atomic<int> children_running{10};

    for (int a = 0; a < 4; ++a) {
        runner.spawn(
            [&] {
                while (!children_done.load()) {
                    tracker.enter("adults");
                    cc.adult_enter();
                    jitter(3);
                    cc.adult_leave();
                    tracker.exit("adults");
                    jitter(1);
                }
            },
            "adult" + std::to_string(a));
    }
    for (int c = 0; c < 10; ++c) {
        runner.spawn(
            [&] {
                struct Dec {
                    std::atomic<int>& n;
                    ~Dec() { n.fetch_sub(1); }
                } dec{children_running};
                for (int i = 0; i < 8; ++i) {
                    cc.child_enter();
                    check_ratio(tracker, tracker.enter("children"),
                                "on child entry");
                    jitter(2);
                    tracker.exit("children");
                    cc.child_leave();
                    jitter(1);
                }
            },
            "child" + std::to_string(c));
    }

    auto deadline = std::chrono::steady_clock::now() + 25s;
    while (children_running.load() > 0) {
        KOAN_ASSERT_MSG(std::chrono::steady_clock::now() < deadline,
                        "the children never finished their visits (deadlock?)");
        check_ratio(tracker, snapshot(tracker), "sampled");
        std::this_thread::sleep_for(2ms);
    }
    children_done.store(true);
    runner.join_all(10000ms);
    tracker.assert_no_violations();
}

KOAN_TEST(child_blocks_without_adults) {
    auto cc = std::make_shared<ExtendedChildCare>(kRatio);
    auto probe = assert_blocks([cc] { cc->child_enter(); }, 300ms,
                               "child_enter with no adult inside");
    assert_completes([cc] { cc->adult_enter(); }, 2000ms, "adult_enter");
    probe.assert_completed(5000ms, "the waiting child once an adult arrives");
}

KOAN_TEST(adult_admits_three) {
    auto cc = std::make_shared<ExtendedChildCare>(kRatio);
    assert_completes([cc] { cc->adult_enter(); }, 2000ms, "adult_enter");
    for (int i = 0; i < kRatio; ++i)
        assert_completes([cc] { cc->child_enter(); }, 2000ms,
                         "child " + std::to_string(i + 1) + " of 3 (they fit)");
    auto probe = assert_blocks(
        [cc] { cc->child_enter(); }, 300ms,
        "child 4 (one adult supervises at most 3)");
    cc->adult_enter();
    probe.assert_completed(5000ms,
                           "the waiting child once a second adult arrives");
}

KOAN_TEST(adult_leave_blocks_while_needed) {
    auto cc = std::make_shared<ExtendedChildCare>(kRatio);
    cc->adult_enter();
    cc->child_enter();
    cc->child_enter();
    auto probe = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                               "adult_leave (2 children need the only adult)");
    assert_completes([cc] { cc->child_leave(); }, 2000ms, "child_leave");
    std::this_thread::sleep_for(150ms);
    KOAN_ASSERT_MSG(!probe.done->load(),
                    "the adult left while 1 child was still inside with "
                    "nobody else");
    cc->child_leave();
    probe.assert_completed(5000ms, "adult_leave once the room empties");
}

// Koan 33's nemesis schedule must still work here: 2 adults, 6 children;
// both adults call adult_leave; children trickle out until exactly one
// departure is legal. Exactly one leaver gets out.
KOAN_TEST(two_adults_leaving_no_deadlock) {
    for (int trial = 0; trial < 3; ++trial) {
        auto cc = std::make_shared<ExtendedChildCare>(kRatio);
        cc->adult_enter();
        cc->adult_enter();
        for (int i = 0; i < 2 * kRatio; ++i)
            assert_completes([cc] { cc->child_enter(); }, 2000ms,
                             "child " + std::to_string(i + 1) +
                                 " of 6 (they fit under 2 adults)");
        auto first = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                                   "adult_leave while 6 children need both");
        auto second = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                                    "adult_leave while 6 children need both");
        for (int i = 0; i < kRatio; ++i) {
            std::this_thread::sleep_for(50ms);
            cc->child_leave();
        }
        eventually(
            [&] { return first.done->load() || second.done->load(); }, 2000ms,
            "trial " + std::to_string(trial) +
                ": 3 children remain, so one adult can leave — but neither "
                "did (did the leavers deadlock each other?)");
        std::this_thread::sleep_for(300ms);
        int left = (first.done->load() ? 1 : 0) + (second.done->load() ? 1 : 0);
        KOAN_ASSERT_MSG(left == 1,
                        "trial " + std::to_string(trial) +
                            ": exactly one adult may leave 3 children with "
                            "one adult, but " + std::to_string(left) +
                            " got out");
        for (int i = 0; i < kRatio; ++i) cc->child_leave();
        auto& remaining = first.done->load() ? second : first;
        remaining.assert_completed(
            5000ms, "the second adult once the center is empty");
    }
}

// ── The new requirement: no unnecessary waiting ──────────────────────────

// THE extended-child-care scenario. 2 adults + 4 children inside; one
// adult gets stuck trying to leave (4 > 3x1). A 5th child arrives.
// 5 <= 3x2 — the stuck adult is still inside, so the child MUST be
// admitted promptly. A solution where the leaver has already pocketed
// part of the capacity turns the child away.
KOAN_TEST(child_admitted_despite_waiting_adult) {
    auto cc = std::make_shared<ExtendedChildCare>(kRatio);
    cc->adult_enter();
    cc->adult_enter();
    for (int i = 0; i < 4; ++i)
        assert_completes([cc] { cc->child_enter(); }, 2000ms,
                         "child " + std::to_string(i + 1) +
                             " of 4 (they fit under 2 adults)");
    auto leaver = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                                "adult_leave (4 children need both adults)");
    assert_completes(
        [cc] { cc->child_enter(); }, 2000ms,
        "a 5th child (5 <= 3x2 — the adult stuck at the door must not bar "
        "the way in)");
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(!leaver.done->load(),
                    "the adult left while 5 children needed both adults");
    for (int i = 0; i < 3; ++i) cc->child_leave();
    leaver.assert_completed(5000ms,
                            "adult_leave once only 2 children remain (2 <= 3x1)");
}

// Continue the scenario: a stuck leaver departs the moment it can.
KOAN_TEST(waiting_adult_leaves_when_kids_drop) {
    auto cc = std::make_shared<ExtendedChildCare>(kRatio);
    cc->adult_enter();
    cc->adult_enter();
    for (int i = 0; i < 5; ++i)
        assert_completes([cc] { cc->child_enter(); }, 2000ms,
                         "child " + std::to_string(i + 1) +
                             " of 5 (they fit under 2 adults)");
    auto leaver = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                                "adult_leave (5 children need both adults)");
    cc->child_leave();  // 4 children: still too many for one adult
    std::this_thread::sleep_for(200ms);
    KOAN_ASSERT_MSG(!leaver.done->load(),
                    "the adult left while 4 children needed both adults");
    cc->child_leave();  // 3 children: 3 <= 3x1 — departure is now legal
    leaver.assert_completed(
        5000ms, "the waiting adult once 3 children fit under the other one");
    auto second = assert_blocks([cc] { cc->adult_leave(); }, 300ms,
                                "the last adult leaving 3 children behind");
    for (int i = 0; i < 3; ++i) cc->child_leave();
    second.assert_completed(5000ms, "the last adult once the center is empty");
}
