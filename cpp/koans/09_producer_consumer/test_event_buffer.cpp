#include "koan_test.hpp"
#include "event_buffer.hpp"

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

// An instrumented buffer that DETECTS concurrent access. add()/get() raise
// an "operation in flight" count on entry, dwell a moment to widen any race
// window, do the real work, then lower it. Two operations in flight at once
// means the user's solution failed to serialize buffer access → violation.
// get() on an empty buffer records a violation and returns kEmpty.
// (The internal mutex only protects the instrumentation and the deque from
// corruption — it deliberately does NOT serialize the dwell, so a
// non-exclusive solution still gets caught.)
class GuardedBuffer {
  public:
    void add(int item) {
        begin("add");
        dwell();
        {
            std::lock_guard lock(mutex_);
            items_.push_back(item);
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
    std::vector<std::string> violations_;
};

struct Session {
    std::shared_ptr<GuardedBuffer> buf;
    std::vector<int> produced;
    std::vector<int> consumed;
};

// n_producers×items_each produces vs n_consumers×items_each consumes.
// Producer counts equal consumer counts, so a correct solution finishes
// every consume.
Session run_session(int n_producers = 3, int n_consumers = 3,
                    int items_each = 40, int max_jitter_ms = 1) {
    Session s;
    s.buf = std::make_shared<GuardedBuffer>();
    ProducerConsumer<GuardedBuffer> pc(*s.buf);
    std::mutex consumed_mutex;
    ThreadRunner runner;
    for (int p = 0; p < n_producers; ++p) {
        runner.spawn(
            [&, p] {
                for (int i = 0; i < items_each; ++i) {
                    jitter(max_jitter_ms);
                    pc.produce(p * 1000 + i);
                }
            },
            "producer-" + std::to_string(p));
    }
    for (int c = 0; c < n_consumers; ++c) {
        runner.spawn(
            [&] {
                for (int i = 0; i < items_each; ++i) {
                    jitter(max_jitter_ms);
                    int item = pc.consume();
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

KOAN_TEST(produce_never_waits_for_a_consumer) {
    auto buf = std::make_shared<GuardedBuffer>();
    auto pc = std::make_shared<ProducerConsumer<GuardedBuffer>>(*buf);
    for (int i = 0; i < 5; ++i)
        assert_completes([pc, buf, i] { pc->produce(i); }, 2000ms,
                         "produce() with no consumer around");
    buf->assert_no_violations();
}

KOAN_TEST(consume_blocks_on_empty_then_gets_the_item) {
    auto buf = std::make_shared<GuardedBuffer>();
    auto pc = std::make_shared<ProducerConsumer<GuardedBuffer>>(*buf);
    auto result = std::make_shared<std::atomic<int>>(kEmpty - 1);
    auto probe = assert_blocks([pc, buf, result] { result->store(pc->consume()); },
                               300ms, "consume() on an empty buffer");
    pc->produce(42);
    probe.assert_completed(5000ms, "consume() once an item is produced");
    KOAN_ASSERT_EQ(result->load(), 42);
    buf->assert_no_violations();
}

// Deliberately NOT checked: FIFO order. The contract makes the buffer a
// bag — any item satisfies a consume. Only the multiset must match.
KOAN_TEST(no_lost_or_duplicated_items) {
    auto s = run_session();
    assert_same_multiset(s);
}

KOAN_TEST(buffer_access_is_exclusive) {
    auto s = run_session();
    s.buf->assert_no_violations();
    assert_same_multiset(s);
}

KOAN_TEST(stress_with_jitter_on_both_sides) {
    for (int round = 0; round < 3; ++round) {
        auto s = run_session(4, 4, 25, 1);
        s.buf->assert_no_violations();
        assert_same_multiset(s);
    }
}
