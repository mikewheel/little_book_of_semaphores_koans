#include "koan_test.hpp"
#include "bounded_buffer.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>
#include <string>
#include <vector>

using namespace koans;

namespace {

// Sentinel returned by GuardedBuffer::get() on an empty buffer, instead of
// crashing the calling thread.
constexpr int kEmpty = -999999;

// An instrumented buffer that DETECTS concurrent access and overfill. Same
// detector as koan 09 (an "operation in flight" count plus a dwell to widen
// race windows), with one addition: it tracks the largest size it ever
// reached, so tests can prove the capacity bound held. get() on an empty
// buffer records a violation and returns kEmpty.
class GuardedBuffer {
  public:
    void add(int item) {
        begin("add");
        dwell();
        {
            std::lock_guard lock(mutex_);
            items_.push_back(item);
            max_size_ = std::max(max_size_, static_cast<int>(items_.size()));
        }
        end();
    }

    int get() {
        begin("get");
        dwell();
        int out;
        {
            std::lock_guard lock(mutex_);
            if (items_.empty()) {
                violations_.push_back("get() called on an empty buffer");
                out = kEmpty;
            } else {
                out = items_.front();
                items_.pop_front();
            }
        }
        end();
        return out;
    }

    int max_size() const {
        std::lock_guard lock(mutex_);
        return max_size_;
    }

    void assert_no_violations() const {
        std::lock_guard lock(mutex_);
        if (!violations_.empty())
            KOAN_FAIL(std::to_string(violations_.size()) +
                      " buffer violation(s); first: " + violations_.front());
    }

  private:
    void begin(const char* op) {
        if (in_flight_.fetch_add(1) > 0) {
            std::lock_guard lock(mutex_);
            violations_.push_back(std::string(op) +
                                  "() overlapped another buffer operation");
        }
    }

    void end() { in_flight_.fetch_sub(1); }

    static void dwell() { std::this_thread::sleep_for(200us); }

    mutable std::mutex mutex_;
    std::deque<int> items_;
    std::atomic<int> in_flight_{0};
    int max_size_ = 0;
    std::vector<std::string> violations_;
};

struct Session {
    std::shared_ptr<GuardedBuffer> buf;
    std::vector<int> produced;
    std::vector<int> consumed;
};

// Equal produce/consume counts; consumers slightly slower by default so
// the buffer runs near-full and the capacity bound actually gets tested.
Session run_session(int capacity, int n_producers = 3, int n_consumers = 3,
                    int items_each = 40, int producer_jitter_ms = 1,
                    int consumer_jitter_ms = 2) {
    Session s;
    s.buf = std::make_shared<GuardedBuffer>();
    BoundedBuffer<GuardedBuffer> bb(*s.buf, capacity);
    std::mutex consumed_mutex;
    ThreadRunner runner;
    for (int p = 0; p < n_producers; ++p) {
        runner.spawn(
            [&, p] {
                for (int i = 0; i < items_each; ++i) {
                    jitter(producer_jitter_ms);
                    bb.produce(p * 1000 + i);
                }
            },
            "producer-" + std::to_string(p));
    }
    for (int c = 0; c < n_consumers; ++c) {
        runner.spawn(
            [&] {
                for (int i = 0; i < items_each; ++i) {
                    jitter(consumer_jitter_ms);
                    int item = bb.consume();
                    std::lock_guard lock(consumed_mutex);
                    s.consumed.push_back(item);
                }
            },
            "consumer-" + std::to_string(c));
    }
    // Below the 20 s per-test watchdog, so a deadlocked solution fails this
    // one test instead of aborting the whole binary.
    runner.join_all(15000ms);
    for (int p = 0; p < n_producers; ++p)
        for (int i = 0; i < items_each; ++i) s.produced.push_back(p * 1000 + i);
    return s;
}

void assert_same_multiset(Session& s) {
    std::sort(s.produced.begin(), s.produced.end());
    std::sort(s.consumed.begin(), s.consumed.end());
    KOAN_ASSERT_MSG(s.consumed == s.produced,
                    "consumed items != produced items (lost, duplicated, or "
                    "empty-buffer sentinel)");
}

}  // namespace

KOAN_TEST(produce_completes_while_there_is_space) {
    auto buf = std::make_shared<GuardedBuffer>();
    auto bb = std::make_shared<BoundedBuffer<GuardedBuffer>>(*buf, 8);
    for (int i = 0; i < 8; ++i)
        assert_completes([bb, buf, i] { bb->produce(i); }, 2000ms,
                         "produce() while the buffer has space");
    buf->assert_no_violations();
}

KOAN_TEST(consume_blocks_on_empty_then_gets_the_item) {
    auto buf = std::make_shared<GuardedBuffer>();
    auto bb = std::make_shared<BoundedBuffer<GuardedBuffer>>(*buf, 4);
    auto result = std::make_shared<std::atomic<int>>(kEmpty - 1);
    auto probe = assert_blocks([bb, buf, result] { result->store(bb->consume()); },
                               300ms, "consume() on an empty buffer");
    bb->produce(42);
    probe.assert_completed(5000ms, "consume() once an item is produced");
    KOAN_ASSERT_EQ(result->load(), 42);
    buf->assert_no_violations();
}

KOAN_TEST(producer_blocks_when_full) {
    auto buf = std::make_shared<GuardedBuffer>();
    auto bb = std::make_shared<BoundedBuffer<GuardedBuffer>>(*buf, 3);
    for (int i = 0; i < 3; ++i)
        assert_completes([bb, buf, i] { bb->produce(i); }, 2000ms,
                         "produce() while the buffer has space");
    auto probe = assert_blocks([bb, buf] { bb->produce(99); }, 300ms,
                               "produce() into a full buffer");
    // A consumer must be able to get in even while a producer is waiting —
    // if this times out, the producer is probably sleeping INSIDE the mutex.
    auto first = std::make_shared<std::atomic<int>>(kEmpty - 1);
    assert_completes([bb, buf, first] { first->store(bb->consume()); }, 5000ms,
                     "consume() while a producer waits for space");
    probe.assert_completed(5000ms, "the waiting producer after a consume");
    std::vector<int> got{first->load()};
    for (int i = 0; i < 3; ++i) {
        auto item = std::make_shared<std::atomic<int>>(kEmpty - 1);
        assert_completes([bb, buf, item] { item->store(bb->consume()); }, 5000ms,
                         "draining the buffer");
        got.push_back(item->load());
    }
    std::sort(got.begin(), got.end());
    KOAN_ASSERT(got == (std::vector<int>{0, 1, 2, 99}));
    KOAN_ASSERT_MSG(buf->max_size() <= 3,
                    "buffer grew to " + std::to_string(buf->max_size()) +
                        " despite capacity 3");
    buf->assert_no_violations();
}

// FIFO order is deliberately not required — only the multiset matters.
KOAN_TEST(no_lost_or_duplicated_items) {
    auto s = run_session(7);
    assert_same_multiset(s);
}

KOAN_TEST(buffer_access_is_exclusive) {
    auto s = run_session(7);
    s.buf->assert_no_violations();
    assert_same_multiset(s);
}

KOAN_TEST(never_exceeds_capacity) {
    auto s = run_session(5, 4, 4, 50, 0, 1);
    KOAN_ASSERT_MSG(s.buf->max_size() <= 5,
                    "buffer reached " + std::to_string(s.buf->max_size()) +
                        " items — capacity 5 was not enforced");
    s.buf->assert_no_violations();
    assert_same_multiset(s);
}

KOAN_TEST(stress_with_jitter_on_both_sides) {
    for (int capacity : {2, 6}) {
        auto s = run_session(capacity, 4, 4, 20, 1, 1);
        KOAN_ASSERT(s.buf->max_size() <= capacity);
        s.buf->assert_no_violations();
        assert_same_multiset(s);
    }
}
