#include "koan_test.hpp"
#include "no_starve_mutex.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

// An increment with a deliberately widened read-modify-write window.
// Relaxed atomics keep the race observable without undefined behavior.
class RacyCounter {
  public:
    void increment(int nap_every, int i) {
        int temp = value_.load(std::memory_order_relaxed);
        if (i % nap_every == 0)
            std::this_thread::sleep_for(500us);  // invite an interleaving
        value_.store(temp + 1, std::memory_order_relaxed);
    }

    int value() const { return value_.load(); }

  private:
    std::atomic<int> value_{0};
};

// A sub-millisecond jitter for high-churn loops.
void tiny_jitter() {
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 300);
    std::this_thread::sleep_for(std::chrono::microseconds(dist(rng)));
}

constexpr int kThreads = 8;
constexpr int kLaps = 250;
// Morris bounds overtaking at roughly two "waiting rooms" worth of threads;
// the slack absorbs scheduling noise around the moment intent is recorded.
constexpr int kBound = 2 * kThreads + 4;

// Worst number of foreign lock grants between the marked thread's declared
// intent (want) and its own grant (got).
int max_overtakes(const std::vector<std::string>& events, int marked) {
    const std::string want = "want:" + std::to_string(marked);
    const std::string got = "got:" + std::to_string(marked);
    int worst = 0, count = 0;
    bool in_window = false;
    for (const auto& e : events) {
        if (e == want) {
            in_window = true;
            count = 0;
        } else if (e == got) {
            if (in_window) worst = std::max(worst, count);
            in_window = false;
        } else if (in_window && e.rfind("got:", 0) == 0) {
            ++count;
        }
    }
    return worst;
}

}  // namespace

// A racy read-modify-write must never lose updates under this lock.
KOAN_TEST(mutual_exclusion) {
    NoStarveMutex m;
    RacyCounter counter;
    constexpr int n_threads = 4, n_iters = 50;
    ThreadRunner runner;
    for (int t = 0; t < n_threads; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < n_iters; ++i) {
                m.acquire();
                counter.increment(10, i);
                m.release();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_MSG(counter.value() == n_threads * n_iters,
                    "lost updates: expected " +
                        std::to_string(n_threads * n_iters) + ", got " +
                        std::to_string(counter.value()) +
                        " — the critical section is not exclusive");
}

KOAN_TEST(second_acquire_blocks_until_release) {
    auto m = std::make_shared<NoStarveMutex>();
    assert_completes([m] { m->acquire(); }, 2000ms, "the first acquire");
    auto probe = assert_blocks([m] { m->acquire(); }, 300ms,
                               "a second acquire (lock is held)");
    m->release();
    probe.assert_completed(5000ms, "the blocked acquire after release");
    m->release();
}

// THE test: with only weak semaphores underneath, nobody may be overtaken
// more than a bounded number of times per acquisition.
KOAN_TEST(bounded_overtaking) {
    NoStarveMutex m;
    EventLog log;
    ThreadRunner runner;
    for (int i = 0; i < kThreads; ++i) {
        runner.spawn(
            [&, i] {
                const std::string want = "want:" + std::to_string(i);
                const std::string got = "got:" + std::to_string(i);
                for (int lap = 0; lap < kLaps; ++lap) {
                    log.record(want);
                    m.acquire();
                    log.record(got);
                    std::this_thread::sleep_for(100us);  // sliver of CS work
                    m.release();
                    tiny_jitter();
                }
            },
            "P" + std::to_string(i));
    }
    runner.join_all(15000ms);

    KOAN_ASSERT_EQ(log.count("got:0"), static_cast<std::size_t>(kLaps));
    int worst = max_overtakes(log.events(), 0);
    KOAN_ASSERT_MSG(worst <= kBound,
                    "thread 0 was overtaken " + std::to_string(worst) +
                        " times while waiting for one acquisition (bound: " +
                        std::to_string(kBound) +
                        ") — a weak semaphore is starving it");
}

// Heavy contention must not deadlock; every thread finishes its laps.
KOAN_TEST(progress_under_contention) {
    NoStarveMutex m;
    int count = 0;  // protected by the lock under test
    constexpr int n_threads = 8, laps = 60;
    ThreadRunner runner;
    for (int t = 0; t < n_threads; ++t) {
        runner.spawn([&] {
            for (int i = 0; i < laps; ++i) {
                m.acquire();
                ++count;
                m.release();
                tiny_jitter();
            }
        });
    }
    runner.join_all(30000ms);
    KOAN_ASSERT_EQ(count, n_threads * laps);
}
