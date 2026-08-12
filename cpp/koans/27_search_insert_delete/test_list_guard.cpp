#include "koan_test.hpp"
#include "list_guard.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

KOAN_TEST(searchers_share) {
    ListGuard guard;
    OverlapTracker tracker;
    std::atomic<bool> release{false};
    ThreadRunner runner;
    for (int i = 0; i < 4; ++i) {
        runner.spawn([&] {
            guard.search_enter();
            tracker.enter("search");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!release.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);  // linger until all inside
            tracker.exit("search");
            guard.search_exit();
        });
    }
    eventually([&] { return tracker.current("search") == 4; }, 5000ms,
               "not all 4 searchers made it inside concurrently — searchers "
               "must not exclude each other");
    release.store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(inserter_with_searchers) {
    auto guard = std::make_shared<ListGuard>();
    auto tracker = std::make_shared<OverlapTracker>();
    std::atomic<bool> release{false};
    ThreadRunner runner;
    for (int i = 0; i < 2; ++i) {
        runner.spawn([&, guard, tracker] {
            guard->search_enter();
            tracker->enter("search");
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!release.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(1ms);
            tracker->exit("search");
            guard->search_exit();
        });
    }
    eventually([tracker] { return tracker->current("search") == 2; }, 5000ms);

    auto log = std::make_shared<EventLog>();
    assert_completes(
        [guard, tracker, log] {
            guard->insert_enter();
            auto snap = tracker->enter("insert");
            tracker->exit("insert");
            guard->insert_exit();
            log->record(snap["search"] == 2 && snap["insert"] == 1
                            ? "witnessed_both_searchers"
                            : "snapshot_wrong");
        },
        5000ms, "an inserter (searchers inside, no deleter)");
    KOAN_ASSERT_MSG(log->count("witnessed_both_searchers") == 1,
                    "the inserter must run while both searchers are inside");
    release.store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(inserters_mutually_exclusive) {
    ListGuard guard;
    OverlapTracker tracker;
    std::atomic<bool> release{false};
    ThreadRunner runner;
    runner.spawn([&] {  // parked searcher: inside for the whole test
        guard.search_enter();
        tracker.enter("search");
        auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!release.load() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        tracker.exit("search");
        guard.search_exit();
    });
    eventually([&] { return tracker.current("search") == 1; }, 5000ms);

    std::atomic<int> inserters_done{0};
    for (int i = 0; i < 2; ++i) {
        runner.spawn([&] {
            for (int k = 0; k < 20; ++k) {
                guard.insert_enter();
                auto snap = tracker.enter("insert");
                if (snap["insert"] > 1)
                    tracker.violate("two inserters inside at once");
                if (snap["search"] < 1)
                    tracker.violate("the parked searcher vanished");
                jitter(1);
                tracker.exit("insert");
                guard.insert_exit();
                jitter();  // give the other inserter a turn
            }
            inserters_done.fetch_add(1);
        });
    }
    eventually([&] { return inserters_done.load() == 2; }, 15000ms,
               "the inserters never finished their loops (blocked by whom?)");
    release.store(true);  // only now may the parked searcher leave
    runner.join_all(5000ms);
    tracker.assert_no_violations();
    KOAN_ASSERT_MSG(tracker.max_concurrent("insert") == 1,
                    "two inserters overlapped");
}

KOAN_TEST(deleter_fully_exclusive) {
    auto guard = std::make_shared<ListGuard>();
    auto tracker = std::make_shared<OverlapTracker>();
    auto log = std::make_shared<EventLog>();

    auto deleter = [guard, tracker, log] {
        guard->delete_enter();
        auto snap = tracker->enter("delete");
        tracker->exit("delete");
        guard->delete_exit();
        log->record(snap["delete"] == 1 && snap["search"] == 0 &&
                            snap["insert"] == 0
                        ? "delete_alone"
                        : "delete_had_company");
    };

    // While a searcher is inside, the deleter must wait.
    guard->search_enter();
    tracker->enter("search");
    auto probe = assert_blocks(deleter, 300ms,
                               "a deleter (a searcher is inside)");
    tracker->exit("search");
    guard->search_exit();
    probe.assert_completed(5000ms, "the deleter once the searcher left");
    KOAN_ASSERT_MSG(log->count("delete_alone") == 1,
                    "the deleter must be alone at entry");

    // While an inserter is inside, the deleter must wait.
    guard->insert_enter();
    tracker->enter("insert");
    probe = assert_blocks(deleter, 300ms,
                          "a deleter (an inserter is inside)");
    tracker->exit("insert");
    guard->insert_exit();
    probe.assert_completed(5000ms, "the deleter once the inserter left");
    KOAN_ASSERT_EQ(log->count("delete_alone"), std::size_t{2});

    // While the deleter is inside, searchers and inserters must wait.
    guard->delete_enter();
    auto search_probe = assert_blocks([guard] { guard->search_enter(); },
                                      300ms,
                                      "a searcher (the deleter is inside)");
    auto insert_probe = assert_blocks([guard] { guard->insert_enter(); },
                                      300ms,
                                      "an inserter (the deleter is inside)");
    guard->delete_exit();
    search_probe.assert_completed(5000ms, "the searcher once the deleter left");
    insert_probe.assert_completed(5000ms, "the inserter once the deleter left");
}

KOAN_TEST(invariant_stress) {
    ListGuard guard;
    OverlapTracker tracker;
    ThreadRunner runner;

    for (int i = 0; i < 4; ++i) {
        runner.spawn([&] {
            for (int k = 0; k < 12; ++k) {
                jitter();
                guard.search_enter();
                auto snap = tracker.enter("search");
                if (snap["delete"] > 0)
                    tracker.violate("searcher entered during a delete");
                jitter(1);
                tracker.exit("search");
                guard.search_exit();
            }
        });
    }
    for (int i = 0; i < 2; ++i) {
        runner.spawn([&] {
            for (int k = 0; k < 12; ++k) {
                jitter();
                guard.insert_enter();
                auto snap = tracker.enter("insert");
                if (snap["insert"] > 1)
                    tracker.violate("two inserters inside at once");
                if (snap["delete"] > 0)
                    tracker.violate("inserter entered during a delete");
                jitter(1);
                tracker.exit("insert");
                guard.insert_exit();
            }
        });
    }
    for (int i = 0; i < 2; ++i) {
        runner.spawn([&] {
            for (int k = 0; k < 12; ++k) {
                jitter();
                guard.delete_enter();
                auto snap = tracker.enter("delete");
                if (snap["search"] > 0 || snap["insert"] > 0 ||
                    snap["delete"] != 1)
                    tracker.violate("deleter was not alone");
                jitter(1);
                tracker.exit("delete");
                guard.delete_exit();
            }
        });
    }
    runner.join_all(20000ms);
    tracker.assert_no_violations();
    KOAN_ASSERT(tracker.max_concurrent("insert") <= 1);
}
