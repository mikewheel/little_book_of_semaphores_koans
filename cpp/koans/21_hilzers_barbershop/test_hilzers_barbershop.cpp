#include "koan_test.hpp"
#include "hilzers_barbershop.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

// Test parameters: a shrunken shop (capacity 8, sofa 3, 2 barbers) so the
// suite stays fast; the book's shop is 20/4/3.
//
// Barber daemons stay parked on semaphores inside the shop forever, so a
// shop can never be safely destroyed. Tests leak each shop on purpose.

namespace {

constexpr int kCapacity = 8;
constexpr int kSofa = 3;
constexpr int kBarbers = 2;

struct Results {
    std::mutex mutex;
    std::map<int, bool> served;
    void set(int cid, bool ok) {
        std::lock_guard lock(mutex);
        served[cid] = ok;
    }
    int count_served() {
        std::lock_guard lock(mutex);
        int n = 0;
        for (auto& [_, ok] : served)
            if (ok) ++n;
        return n;
    }
    std::size_t size() {
        std::lock_guard lock(mutex);
        return served.size();
    }
};

void spawn_customers(ThreadRunner& runner, HilzersBarbershop* shop, int count,
                     std::shared_ptr<Results> results, int stagger_ms = 0) {
    for (int cid = 0; cid < count; ++cid) {
        runner.spawn(
            [shop, results, cid, stagger_ms] {
                if (stagger_ms)
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(stagger_ms * cid));
                jitter();
                results->set(cid, shop->customer_visit(cid));
            },
            "customer-" + std::to_string(cid));
    }
}

}  // namespace

KOAN_TEST(capacity_respected) {
    auto tracker = std::make_shared<OverlapTracker>();
    HilzerHooks hooks;
    hooks.enter_shop = [tracker](int) {
        auto snapshot = tracker->enter("in_shop");
        if (snapshot["in_shop"] > kCapacity)
            tracker->violate(std::to_string(snapshot["in_shop"]) +
                             " customers inside a shop of capacity " +
                             std::to_string(kCapacity));
    };
    hooks.pay = [tracker](int) {
        // Slight under-measurement (the customer stays until the receipt),
        // but exit-at-pay can never overcount — see README contract.
        std::this_thread::sleep_for(2ms);
        tracker->exit("in_shop");
    };
    auto* shop = new HilzersBarbershop(kCapacity, kSofa, kBarbers, hooks);
    shop->start_barbers();
    auto results = std::make_shared<Results>();
    ThreadRunner runner;
    spawn_customers(runner, shop, 14, results);
    runner.join_all(20000ms);

    tracker->assert_no_violations();
    KOAN_ASSERT(tracker->max_concurrent("in_shop") <= kCapacity);
}

KOAN_TEST(sofa_capacity) {
    auto tracker = std::make_shared<OverlapTracker>();
    HilzerHooks hooks;
    hooks.sit_on_sofa = [tracker](int) {
        auto snapshot = tracker->enter("on_sofa");
        if (snapshot["on_sofa"] > kSofa)
            tracker->violate(std::to_string(snapshot["on_sofa"]) +
                             " customers on a sofa that seats " +
                             std::to_string(kSofa));
    };
    hooks.sit_in_chair = [tracker](int) {
        std::this_thread::sleep_for(3ms);  // dwell in the chair phase
        tracker->exit("on_sofa");  // seat frees when this hook returns
    };
    auto* shop = new HilzersBarbershop(kCapacity, kSofa, kBarbers, hooks);
    shop->start_barbers();
    auto results = std::make_shared<Results>();
    ThreadRunner runner;
    spawn_customers(runner, shop, 12, results);
    runner.join_all(20000ms);

    tracker->assert_no_violations();
    KOAN_ASSERT(tracker->max_concurrent("on_sofa") <= kSofa);
}

KOAN_TEST(concurrent_haircuts_up_to_barbers) {
    // Two customers hold their chairs until the test has seen both seated
    // at once: with 2 barbers this MUST happen — a solution that
    // serializes the barbers never reaches it and fails here.
    auto tracker = std::make_shared<OverlapTracker>();
    auto all_in = std::make_shared<std::atomic<bool>>(false);
    HilzerHooks hooks;
    hooks.sit_in_chair = [tracker, all_in](int) {
        tracker->enter("in_chair");
        auto deadline = std::chrono::steady_clock::now() + 5s;
        while (!all_in->load() &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);  // linger until both seated
        tracker->exit("in_chair");
    };
    auto* shop = new HilzersBarbershop(kCapacity, kSofa, kBarbers, hooks);
    shop->start_barbers();
    auto results = std::make_shared<Results>();
    ThreadRunner runner;
    spawn_customers(runner, shop, kBarbers, results);
    eventually([tracker] { return tracker->current("in_chair") == kBarbers; },
               5000ms,
               "never saw " + std::to_string(kBarbers) +
                   " concurrent haircuts — barbers must work in parallel");
    all_in->store(true);
    runner.join_all(15000ms);
    KOAN_ASSERT_EQ(tracker->max_concurrent("in_chair"), kBarbers);
}

KOAN_TEST(sofa_is_fifo) {
    // Six customers take an extra-wide sofa 25 ms apart while the barber
    // is not yet working; once the barber starts, chairs must be offered
    // in sofa-seating order. ONE barber here: with several, two chairs are
    // granted concurrently and the hook-call order between them is an
    // honest race even for correct solutions — a single barber makes the
    // grant order (the property under test) observable.
    for (int trial = 0; trial < 2; ++trial) {
        auto order = std::make_shared<EventLog>();
        HilzerHooks hooks;
        hooks.sit_on_sofa = [order](int cid) {
            order->record("sofa" + std::to_string(cid));
        };
        hooks.sit_in_chair = [order](int cid) {
            order->record("chair" + std::to_string(cid));
        };
        auto* shop = new HilzersBarbershop(kCapacity, 6, 1, hooks);
        auto results = std::make_shared<Results>();
        ThreadRunner runner;
        spawn_customers(runner, shop, 6, results, 25);

        auto sofa_count = [order] {
            int n = 0;
            for (const auto& e : order->events())
                if (e.rfind("sofa", 0) == 0) ++n;
            return n;
        };
        eventually([&] { return sofa_count() == 6; }, 5000ms,
                   "6 customers never made the sofa");
        std::this_thread::sleep_for(150ms);
        for (const auto& e : order->events())
            KOAN_ASSERT_MSG(e.rfind("chair", 0) != 0,
                            "trial " + std::to_string(trial) +
                                ": a chair was taken before any barber "
                                "existed");

        shop->start_barbers();
        runner.join_all(10000ms);
        std::vector<std::string> sofa_order, chair_order;
        for (const auto& e : order->events()) {
            if (e.rfind("sofa", 0) == 0) sofa_order.push_back(e.substr(4));
            if (e.rfind("chair", 0) == 0) chair_order.push_back(e.substr(5));
        }
        KOAN_ASSERT_MSG(chair_order == sofa_order,
                        "trial " + std::to_string(trial) +
                            ": chair order != sofa order (" + order->joined() +
                            ") — the longest-seated customer goes first");
    }
}

KOAN_TEST(payment_pairing) {
    // Customers one at a time: each pay(cid) must be answered by an
    // accept_payment before that customer_visit returns.
    auto log = std::make_shared<EventLog>();
    HilzerHooks hooks;
    hooks.pay = [log](int cid) { log->record("pay" + std::to_string(cid)); };
    hooks.accept_payment = [log](int) { log->record("accept"); };
    auto* shop = new HilzersBarbershop(kCapacity, kSofa, kBarbers, hooks);
    shop->start_barbers();
    for (int cid = 0; cid < 4; ++cid) {
        auto ok = std::make_shared<std::atomic<bool>>(false);
        assert_completes([shop, cid, ok] { ok->store(shop->customer_visit(cid)); },
                         5000ms,
                         "customer " + std::to_string(cid) + "'s visit");
        KOAN_ASSERT_MSG(ok->load(), "an in-capacity customer balked");
        log->record("return" + std::to_string(cid));
    }

    auto events = log->events();
    for (int cid = 0; cid < 4; ++cid) {
        std::size_t pay_i = 0, ret_i = 0;
        for (std::size_t i = 0; i < events.size(); ++i) {
            if (events[i] == "pay" + std::to_string(cid)) pay_i = i;
            if (events[i] == "return" + std::to_string(cid)) ret_i = i;
        }
        bool accepted = false;
        for (std::size_t i = pay_i + 1; i < ret_i; ++i)
            if (events[i] == "accept") accepted = true;
        KOAN_ASSERT_MSG(accepted,
                        "customer " + std::to_string(cid) +
                            " returned without a barber accepting the "
                            "payment: " + log->joined());
    }
    KOAN_ASSERT_EQ(log->count("accept"), static_cast<std::size_t>(4));
}

KOAN_TEST(everyone_served_or_balked) {
    // The stress test: 14 customers vs capacity 8, everything instrumented
    // at once — capacity, sofa, chair ceiling, register exclusivity.
    auto tracker = std::make_shared<OverlapTracker>();
    HilzerHooks hooks;
    hooks.enter_shop = [tracker](int) {
        auto snapshot = tracker->enter("in_shop");
        if (snapshot["in_shop"] > kCapacity)
            tracker->violate(std::to_string(snapshot["in_shop"]) +
                             " in shop of " + std::to_string(kCapacity));
    };
    hooks.sit_on_sofa = [tracker](int) {
        auto snapshot = tracker->enter("on_sofa");
        if (snapshot["on_sofa"] > kSofa)
            tracker->violate(std::to_string(snapshot["on_sofa"]) +
                             " on sofa of " + std::to_string(kSofa));
    };
    hooks.sit_in_chair = [tracker](int) {
        auto snapshot = tracker->enter("in_chair");
        if (snapshot["in_chair"] > kBarbers)
            tracker->violate(std::to_string(snapshot["in_chair"]) +
                             " haircuts with " + std::to_string(kBarbers) +
                             " barbers");
        std::this_thread::sleep_for(5ms);
        tracker->exit("in_chair");
        tracker->exit("on_sofa");
    };
    hooks.pay = [tracker](int) { tracker->exit("in_shop"); };
    hooks.accept_payment = [tracker](int) {
        auto snapshot = tracker->enter("register");
        if (snapshot["register"] > 1)
            tracker->violate("two barbers at the one cash register");
        std::this_thread::sleep_for(2ms);
        tracker->exit("register");
    };
    auto* shop = new HilzersBarbershop(kCapacity, kSofa, kBarbers, hooks);
    shop->start_barbers();
    auto results = std::make_shared<Results>();
    ThreadRunner runner;
    spawn_customers(runner, shop, 14, results);
    runner.join_all(20000ms);

    tracker->assert_no_violations();
    KOAN_ASSERT_MSG(results->size() == 14,
                    "every visit must return true or false");
    KOAN_ASSERT_MSG(results->count_served() >= kCapacity,
                    "only " + std::to_string(results->count_served()) +
                        " of 14 served — at least " +
                        std::to_string(kCapacity) + " always fit");
    KOAN_ASSERT(tracker->max_concurrent("in_shop") <= kCapacity);
    KOAN_ASSERT(tracker->max_concurrent("on_sofa") <= kSofa);
    KOAN_ASSERT(tracker->max_concurrent("in_chair") <= kBarbers);
    KOAN_ASSERT(tracker->max_concurrent("register") <= 1);
}
