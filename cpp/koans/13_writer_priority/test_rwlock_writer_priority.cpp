#include "koan_test.hpp"
#include "rwlock_writer_priority.hpp"

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

// The ambush: W1 queues, THEN late readers arrive, THEN W2 queues. Writer
// priority demands both writers finish before any late reader enters —
// even W2, who arrived after the readers. A one-for-one fair gate
// (koan 12) lets a reader slip in between W1 and W2 here. Returns the log.
std::shared_ptr<EventLog> writer_convoy_trial() {
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
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

    auto writer = [lock, log](const std::string& name) {
        lock->writer_enter();
        log->record("writer_in");
        jitter(1);
        lock->writer_exit();
        log->record(name + "_done");
    };

    // W1 arrives and blocks (incumbent readers still inside).
    auto w1_probe = assert_blocks([writer] { writer("w1"); }, 300ms,
                                  "W1 (incumbent readers still reading)");

    // Late readers arrive AFTER W1 — they must be barred already.
    for (int i = 0; i < 3; ++i) {
        runner.spawn(
            [lock, log] {
                lock->reader_enter();
                log->record("late_reader_in");
                lock->reader_exit();
            },
            "late-reader-" + std::to_string(i));
    }
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(log->count("late_reader_in") == 0,
                    "readers entered while a writer was queued");

    // W2 arrives AFTER the late readers. Priority says: still before them.
    auto w2_probe = assert_blocks([writer] { writer("w2"); }, 300ms,
                                  "W2 (incumbents still reading)");

    release_incumbents->store(true);
    w1_probe.assert_completed(5000ms, "W1 once the incumbents left");
    w2_probe.assert_completed(5000ms, "W2 in the same writer convoy");
    log->wait_for_count("late_reader_in", 3, 5000ms);
    runner.join_all(5000ms);
    KOAN_ASSERT_EQ(log->count("writer_in"), 2u);
    // THE priority check: every writer entry precedes every late reader.
    log->assert_before("writer_in", "late_reader_in");
    return log;
}

}  // namespace

KOAN_TEST(readers_barred_while_writers_queued) {
    for (int trial = 0; trial < 5; ++trial) writer_convoy_trial();
}

KOAN_TEST(readers_run_after_writers_drain) {
    auto log = writer_convoy_trial();
    KOAN_ASSERT_EQ(log->count("late_reader_in"), 3u);
    KOAN_ASSERT_EQ(log->count("w1_done"), 1u);
    KOAN_ASSERT_EQ(log->count("w2_done"), 1u);
}

// No writer around: four readers must still hold the lock together.
KOAN_TEST(concurrency_preserved) {
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
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
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
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
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
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

KOAN_TEST(writers_exclude_each_other) {
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
    assert_completes([lock] { lock->writer_enter(); }, 2000ms, "the first writer");
    auto probe = assert_blocks([lock] { lock->writer_enter(); }, 300ms,
                               "a second writer_enter");
    lock->writer_exit();
    probe.assert_completed(5000ms, "the second writer after the first left");
    lock->writer_exit();
}

// Safety under churn — writers alone, readers overlapping. Writer streams
// are finite, so readers can't starve forever here; the point is that
// snapshots never show a writer sharing the room.
KOAN_TEST(invariant_stress) {
    auto lock = std::make_shared<WriterPriorityReadWriteLock>();
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
