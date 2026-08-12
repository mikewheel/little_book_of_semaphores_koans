#include "koan_test.hpp"
#include "rwlock.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace koans;

namespace {

// Park until `flag` is set (or the deadline passes — a safety valve so a
// failed test doesn't strand workers forever).
void linger_until(const std::shared_ptr<std::atomic<bool>>& flag,
                  std::chrono::milliseconds limit = 5000ms) {
    auto deadline = std::chrono::steady_clock::now() + limit;
    while (!flag->load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
}

}  // namespace

// Four readers must be able to hold the lock at the same time.
KOAN_TEST(readers_share) {
    auto lock = std::make_shared<ReadWriteLock>();
    auto tracker = std::make_shared<OverlapTracker>();
    auto all_in = std::make_shared<std::atomic<bool>>(false);
    ThreadRunner runner;
    for (int i = 0; i < 4; ++i) {
        runner.spawn([lock, tracker, all_in] {
            lock->reader_enter();
            tracker->enter("read");
            linger_until(all_in);  // stay inside until everyone has arrived
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
    auto lock = std::make_shared<ReadWriteLock>();
    auto log = std::make_shared<EventLog>();
    assert_completes([lock] { lock->writer_enter(); }, 2000ms,
                     "a writer entering an idle lock");

    auto probe = assert_blocks(
        [lock, log] {
            lock->reader_enter();
            log->record("probe_reader_in");
        },
        300ms, "reader_enter (a writer holds the lock)");

    ThreadRunner runner;
    for (int i = 0; i < 2; ++i) {
        runner.spawn([lock, log, i] {
            lock->reader_enter();
            log->record("reader" + std::to_string(i) + "_in");
            lock->reader_exit();
        });
    }
    std::this_thread::sleep_for(250ms);  // every chance to sneak in
    KOAN_ASSERT_MSG(log->events().empty(),
                    "readers entered over a live writer: " + log->joined());

    lock->writer_exit();
    probe.assert_completed(5000ms, "the blocked reader after writer_exit");
    log->wait_for_count("reader0_in", 1, 5000ms);
    log->wait_for_count("reader1_in", 1, 5000ms);
    lock->reader_exit();  // the probe's reader
    runner.join_all(5000ms);
}

KOAN_TEST(readers_exclude_writer) {
    auto lock = std::make_shared<ReadWriteLock>();
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
    auto lock = std::make_shared<ReadWriteLock>();
    assert_completes([lock] { lock->writer_enter(); }, 2000ms, "the first writer");
    auto probe = assert_blocks([lock] { lock->writer_enter(); }, 300ms,
                               "a second writer_enter");
    lock->writer_exit();
    probe.assert_completed(5000ms, "the second writer after the first left");
    lock->writer_exit();
}

// Snapshots taken at every entry: writers are always alone.
KOAN_TEST(invariant_stress) {
    auto lock = std::make_shared<ReadWriteLock>();
    auto tracker = std::make_shared<OverlapTracker>();
    ThreadRunner runner;
    for (int r = 0; r < 6; ++r) {
        runner.spawn([lock, tracker] {
            for (int i = 0; i < 12; ++i) {
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
            for (int i = 0; i < 8; ++i) {
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
    KOAN_ASSERT_MSG(tracker->max_concurrent("read") >= 2,
                    "readers never actually overlapped — suspiciously "
                    "serialized");
}
