#include "koan_test.hpp"
#include "rwlock_fair.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

using namespace koans;

namespace {

// Park until `flag` is set (or the deadline passes — a safety valve so a
// failed test doesn't strand workers forever).
void linger_until(const std::shared_ptr<std::atomic<bool>>& flag,
                  std::chrono::milliseconds limit = 10000ms) {
    auto deadline = std::chrono::steady_clock::now() + limit;
    while (!flag->load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
}

// One full scene: incumbents in, writer queues, latecomers must wait.
// Returns the EventLog for extra assertions.
std::shared_ptr<EventLog> no_starvation_trial() {
    auto lock = std::make_shared<NoStarveReadWriteLock>();
    auto log = std::make_shared<EventLog>();
    auto release_incumbents = std::make_shared<std::atomic<bool>>(false);
    ThreadRunner runner;

    for (int i = 0; i < 2; ++i) {
        runner.spawn(
            [lock, log, release_incumbents] {
                lock->reader_enter();
                log->record("incumbent_in");
                linger_until(release_incumbents);
                lock->reader_exit();
            },
            "incumbent-" + std::to_string(i));
    }
    log->wait_for_count("incumbent_in", 2, 5000ms);

    // A writer arrives. It must block — two readers are mid-read.
    auto writer_probe = assert_blocks(
        [lock, log] {
            lock->writer_enter();
            log->record("writer_in");
        },
        300ms, "writer_enter (incumbent readers still reading)");

    // Three readers arrive AFTER the writer. Fairness says they queue.
    for (int i = 0; i < 3; ++i) {
        runner.spawn(
            [lock, log] {
                lock->reader_enter();
                log->record("late_reader_in");
                lock->reader_exit();
            },
            "late-reader-" + std::to_string(i));
    }
    std::this_thread::sleep_for(300ms);  // every chance to jump the queue
    KOAN_ASSERT_MSG(log->count("late_reader_in") == 0,
                    "readers that arrived after a waiting writer entered "
                    "before it");

    // Incumbents leave; the writer — not the latecomers — goes next.
    release_incumbents->store(true);
    writer_probe.assert_completed(5000ms,
                                  "the writer once the incumbents left");
    KOAN_ASSERT_MSG(log->count("late_reader_in") == 0,
                    "a late reader entered before the queued writer");

    lock->writer_exit();
    log->wait_for_count("late_reader_in", 3, 5000ms);
    runner.join_all(5000ms);
    log->assert_before("writer_in", "late_reader_in");
    return log;
}

}  // namespace

KOAN_TEST(writer_not_starved) {
    for (int trial = 0; trial < 5; ++trial) no_starvation_trial();
}

KOAN_TEST(late_readers_eventually_get_in) {
    auto log = no_starvation_trial();
    KOAN_ASSERT_EQ(log->count("late_reader_in"), 3u);
    KOAN_ASSERT_EQ(log->count("writer_in"), 1u);
}

// No writer around: four readers must still hold the lock together.
KOAN_TEST(readers_share) {
    auto lock = std::make_shared<NoStarveReadWriteLock>();
    auto tracker = std::make_shared<OverlapTracker>();
    auto all_in = std::make_shared<std::atomic<bool>>(false);
    ThreadRunner runner;
    for (int i = 0; i < 4; ++i) {
        runner.spawn([lock, tracker, all_in] {
            lock->reader_enter();
            tracker->enter("read");
            linger_until(all_in, 5000ms);
            tracker->exit("read");
            lock->reader_exit();
        });
    }
    eventually([tracker] { return tracker->current("read") == 4; }, 5000ms,
               "only " + std::to_string(tracker->current("read")) +
                   " of 4 readers inside together — readers are being "
                   "serialized");
    all_in->store(true);
    runner.join_all(5000ms);
}

KOAN_TEST(writer_excludes_readers) {
    auto lock = std::make_shared<NoStarveReadWriteLock>();
    auto log = std::make_shared<EventLog>();
    assert_completes([lock] { lock->writer_enter(); }, 2000ms,
                     "a writer entering an idle lock");
    auto probe = assert_blocks(
        [lock, log] {
            lock->reader_enter();
            log->record("reader_in");
        },
        300ms, "reader_enter (a writer holds the lock)");
    lock->writer_exit();
    probe.assert_completed(5000ms, "the reader after writer_exit");
    lock->reader_exit();
}

KOAN_TEST(readers_exclude_writer) {
    auto lock = std::make_shared<NoStarveReadWriteLock>();
    auto log = std::make_shared<EventLog>();
    for (int i = 0; i < 2; ++i)
        assert_completes([lock] { lock->reader_enter(); }, 2000ms,
                         "a reader entering an idle lock");
    auto probe = assert_blocks(
        [lock, log] {
            lock->writer_enter();
            log->record("writer_in");
        },
        300ms, "writer_enter (readers hold the lock)");
    lock->reader_exit();
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("writer_in") == 0,
                    "the writer entered while one reader was still inside");
    lock->reader_exit();
    probe.assert_completed(5000ms, "the writer after the LAST reader left");
    lock->writer_exit();
}

// Safety under churn: every writer-entry snapshot shows an empty room.
KOAN_TEST(invariant_stress) {
    auto lock = std::make_shared<NoStarveReadWriteLock>();
    auto tracker = std::make_shared<OverlapTracker>();
    ThreadRunner runner;
    for (int r = 0; r < 6; ++r) {
        runner.spawn([lock, tracker] {
            for (int i = 0; i < 10; ++i) {
                jitter();
                lock->reader_enter();
                auto snap = tracker->enter("read");
                if (snap["write"] > 0)
                    tracker->violate("reader entered alongside a writer");
                jitter(1);
                tracker->exit("read");
                lock->reader_exit();
            }
        });
    }
    for (int w = 0; w < 3; ++w) {
        runner.spawn([lock, tracker] {
            for (int i = 0; i < 6; ++i) {
                jitter();
                lock->writer_enter();
                auto snap = tracker->enter("write");
                if (snap["read"] > 0 || snap["write"] != 1)
                    tracker->violate("writer entered a non-empty room");
                jitter(1);
                tracker->exit("write");
                lock->writer_exit();
            }
        });
    }
    runner.join_all(15000ms);
    tracker->assert_no_violations();
    KOAN_ASSERT(tracker->max_concurrent("write") <= 1);
}
