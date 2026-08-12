#include "koan_test.hpp"
#include "barbershop.hpp"

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

// Note: this shop makes NO promise about serving customers in arrival
// order — any waiting customer may be called next. Koan 20 adds FIFO.
//
// The barber daemon stays parked on a semaphore inside the Barbershop
// forever, so the shop can never be safely destroyed. Tests leak it on
// purpose; the process exits after the run anyway.

namespace {

int random_ms(int max_ms) {
    thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, max_ms);
    return dist(rng);
}

}  // namespace

KOAN_TEST(haircut_pairing) {
    // 10 customers stream through a 4-seat shop (a test-side gate keeps at
    // most 4 in flight, so nobody balks). Every cut_hair() must overlap
    // exactly one customer's get_hair_cut() — never two chairs at once, and
    // never a cut that races ahead of the customer it belongs to.
    auto* shop = new Barbershop(4);  // leaked on purpose (see header note)
    auto tracker = std::make_shared<OverlapTracker>();
    auto gate = std::make_shared<std::counting_semaphore<>>(4);
    auto served = std::make_shared<std::atomic<int>>(0);

    auto cut_hair = [tracker] {
        // NB: this runs on the user's barber thread, so it must never
        // throw — it records violations for the test thread to assert on.
        tracker->enter("cutting");
        // The paired customer must show up in the chair while we cut...
        auto deadline = std::chrono::steady_clock::now() + 5s;
        while (tracker->current("being_cut") < 1 &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        if (tracker->current("being_cut") != 1) {
            tracker->violate("cut_hair ran without exactly one customer in "
                             "the chair");
        } else {
            std::this_thread::sleep_for(2ms);
            // ...and still be the only one there while we're cutting.
            if (tracker->current("being_cut") != 1)
                tracker->violate(
                    "a second customer was in a chair before the previous "
                    "haircut was fully done");
        }
        tracker->exit("cutting");
    };
    auto get_hair_cut = [tracker] {
        auto snapshot = tracker->enter("being_cut");
        if (snapshot["being_cut"] > 1)
            tracker->violate("two customers being cut at once");
        std::this_thread::sleep_for(10ms);
        tracker->exit("being_cut");
    };

    shop->start_barber(cut_hair);
    ThreadRunner runner;
    for (int i = 0; i < 10; ++i) {
        runner.spawn(
            [shop, gate, get_hair_cut, served] {
                gate->acquire();
                bool ok = shop->customer_visit(get_hair_cut);
                gate->release();
                if (ok) served->fetch_add(1);
            },
            "customer");
    }
    runner.join_all(15000ms);

    tracker->assert_no_violations();
    KOAN_ASSERT_MSG(served->load() == 10,
                    "every gated customer should be served; got " +
                        std::to_string(served->load()));
    KOAN_ASSERT_EQ(tracker->max_concurrent("being_cut"), 1);
    KOAN_ASSERT_EQ(tracker->max_concurrent("cutting"), 1);
}

KOAN_TEST(balk_when_full) {
    constexpr int n = 4;
    auto* shop = new Barbershop(n);  // leaked on purpose
    auto haircuts_may_finish = std::make_shared<std::atomic<bool>>(false);
    auto cuts = std::make_shared<OverlapTracker>();
    auto served = std::make_shared<std::atomic<int>>(0);

    auto cut_hair = [haircuts_may_finish, cuts] {
        cuts->enter("cut");
        auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!haircuts_may_finish->load() &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        cuts->exit("cut");
    };

    shop->start_barber(cut_hair);
    ThreadRunner runner;
    for (int i = 0; i < n; ++i) {
        runner.spawn(
            [shop, served] {
                if (shop->customer_visit([] {})) served->fetch_add(1);
            },
            "customer");
    }
    // Wait until the shop is genuinely wedged: barber mid-cut, everyone in.
    eventually([cuts] { return cuts->current("cut") == 1; }, 5000ms,
               "the barber never started cutting");
    std::this_thread::sleep_for(300ms);  // let all n finish checking in

    // The (n+1)th customer must bounce straight off the full shop: a prompt
    // false, not a blocked call.
    auto balked = std::make_shared<std::atomic<int>>(0);  // 0=?, 1=balk, 2=served
    assert_completes(
        [shop, balked] {
            balked->store(shop->customer_visit([] {}) ? 2 : 1);
        },
        2000ms, "customer n+1 (should balk immediately, not block)");
    KOAN_ASSERT_MSG(balked->load() == 1,
                    "customer n+1 should have balked with false");

    haircuts_may_finish->store(true);  // unblock the barber → insiders drain
    runner.join_all(10000ms);
    KOAN_ASSERT_MSG(served->load() == n,
                    "all " + std::to_string(n) + " insiders should be "
                    "served; got " + std::to_string(served->load()));
}

KOAN_TEST(barber_sleeps_when_no_customers) {
    auto* shop = new Barbershop(4);  // leaked on purpose
    auto cuts = std::make_shared<std::atomic<int>>(0);
    shop->start_barber([cuts] { cuts->fetch_add(1); });
    std::this_thread::sleep_for(300ms);  // an empty shop…
    KOAN_ASSERT_MSG(cuts->load() == 0,
                    "the barber cut hair with no customer in the shop");
}

KOAN_TEST(stress_random_arrivals) {
    constexpr int n = 4;
    auto* shop = new Barbershop(n);  // leaked on purpose
    auto tracker = std::make_shared<OverlapTracker>();
    auto cut_count = std::make_shared<std::atomic<int>>(0);
    auto served = std::make_shared<std::atomic<int>>(0);
    auto balked = std::make_shared<std::atomic<int>>(0);

    auto cut_hair = [tracker, cut_count] {
        tracker->enter("cutting");
        cut_count->fetch_add(1);
        std::this_thread::sleep_for(1ms);
        tracker->exit("cutting");
    };
    auto get_hair_cut = [tracker] {
        auto snapshot = tracker->enter("being_cut");
        if (snapshot["being_cut"] > 1)
            tracker->violate("two customers being cut at once");
        std::this_thread::sleep_for(1ms);
        tracker->exit("being_cut");
    };

    shop->start_barber(cut_hair);
    ThreadRunner runner;
    for (int i = 0; i < 20; ++i) {
        runner.spawn(
            [shop, get_hair_cut, served, balked] {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(random_ms(50)));
                jitter();
                if (shop->customer_visit(get_hair_cut))
                    served->fetch_add(1);
                else
                    balked->fetch_add(1);
            },
            "customer");
    }
    runner.join_all(15000ms);

    tracker->assert_no_violations();
    KOAN_ASSERT_EQ(served->load() + balked->load(), 20);
    KOAN_ASSERT_MSG(served->load() >= n,
                    "only " + std::to_string(served->load()) +
                        " customers served out of 20");
    KOAN_ASSERT(tracker->max_concurrent("being_cut") <= 1);
    KOAN_ASSERT_MSG(cut_count->load() == served->load(),
                    std::to_string(cut_count->load()) + " cuts for " +
                        std::to_string(served->load()) +
                        " served customers — cuts and customers must pair 1:1");
}
