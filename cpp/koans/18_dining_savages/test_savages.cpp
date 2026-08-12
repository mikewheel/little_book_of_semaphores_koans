#include "koan_test.hpp"
#include "savages.hpp"

#include <atomic>
#include <string>
#include <thread>

using namespace koans;

namespace {

// A pot that notices misuse. Deliberately NOT thread-safe: serializing
// access to the pot is the solution's job, so a race here surfaces as a
// violation or a wrong count rather than being papered over by a lock.
// (Counters are atomics only so the test thread can read them safely.)
class InstrumentedPot : public Pot {
  public:
    explicit InstrumentedPot(int m) : m_(m) {}

    void put_servings(int m) override {
        int s = servings_.load();
        if (s != 0)
            tracker_.violate("cook refilled a pot that still held " +
                             std::to_string(s) + " serving(s)");
        if (m != m_)
            tracker_.violate("cook refilled with " + std::to_string(m) +
                             ", expected " + std::to_string(m_));
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        servings_.store(m);
        refill_count_.fetch_add(1);
    }

    void get_serving() override {
        int s = servings_.load();
        if (s <= 0) tracker_.violate("a diner took a serving from an empty pot");
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        servings_.store(s - 1);
        served_count_.fetch_add(1);
    }

    int refill_count() const { return refill_count_.load(); }
    int served_count() const { return served_count_.load(); }
    void assert_no_violations() const { tracker_.assert_no_violations(); }

  private:
    int m_;
    std::atomic<int> servings_{0};
    std::atomic<int> refill_count_{0};
    std::atomic<int> served_count_{0};
    OverlapTracker tracker_;  // reused for its violation list
};

// The cook daemon stays parked on a semaphore inside the Village forever,
// so neither the Village nor its pot can ever be safely destroyed. Tests
// leak both on purpose; the process exits after the run anyway.
InstrumentedPot* run_village(int m, int n_savages, int meals_each,
                             int max_jitter_ms = 1) {
    auto* pot = new InstrumentedPot(m);
    auto* village = new Village(m, *pot);
    village->start_cook();
    ThreadRunner runner;
    for (int s = 0; s < n_savages; ++s) {
        runner.spawn(
            [village, meals_each, max_jitter_ms] {
                for (int i = 0; i < meals_each; ++i) {
                    jitter(max_jitter_ms);
                    village->dine();
                }
            },
            "savage");
    }
    runner.join_all(20000ms);
    return pot;
}

}  // namespace

KOAN_TEST(pot_never_misused) {
    auto* pot = run_village(4, 6, 10);
    pot->assert_no_violations();
}

KOAN_TEST(all_meals_served) {
    auto* pot = run_village(4, 6, 10);
    KOAN_ASSERT_MSG(pot->served_count() == 60,
                    "expected 60 servings taken, saw " +
                        std::to_string(pot->served_count()));
}

KOAN_TEST(cook_called_right_number_of_times) {
    // 60 one-serving meals from a pot of 4 → exactly ceil(60/4) == 15
    // refills: one each time a diner finds the pot empty, never on spec.
    auto* pot = run_village(4, 6, 10);
    KOAN_ASSERT_MSG(pot->refill_count() == 15,
                    "expected exactly 15 refills for 60 meals (m=4), saw " +
                        std::to_string(pot->refill_count()) +
                        " — is the cook refilling only on demand?");
}

KOAN_TEST(cook_sleeps_until_needed) {
    auto* pot = new InstrumentedPot(4);
    auto* village = new Village(4, *pot);  // leaked on purpose (see above)
    village->start_cook();
    std::this_thread::sleep_for(300ms);  // nobody is hungry yet
    KOAN_ASSERT_MSG(pot->refill_count() == 0,
                    "the cook refilled the pot before any diner asked — he "
                    "should sleep");
    // The first diner finds the pot empty and must wake the cook.
    assert_completes([village] { village->dine(); }, 5000ms,
                     "the first dine()");
    KOAN_ASSERT_EQ(pot->refill_count(), 1);
    KOAN_ASSERT_EQ(pot->served_count(), 1);
}

KOAN_TEST(stress_with_jitter) {
    // 40 meals from a pot of 3 → exactly ceil(40/3) == 14 refills.
    auto* pot = run_village(3, 8, 5, 2);
    pot->assert_no_violations();
    KOAN_ASSERT_EQ(pot->served_count(), 40);
    KOAN_ASSERT_MSG(pot->refill_count() == 14,
                    "expected exactly 14 refills for 40 meals (m=3), saw " +
                        std::to_string(pot->refill_count()));
}
