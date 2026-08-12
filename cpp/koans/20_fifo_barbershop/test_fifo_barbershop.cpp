#include "koan_test.hpp"
#include "fifo_barbershop.hpp"

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

// The first four tests re-check everything koan 19 demanded — the FIFO
// shop must not lose any of those properties. The last one checks the new
// promise: service order == arrival order.
//
// The barber daemon stays parked on a semaphore inside the shop forever,
// so the shop can never be safely destroyed. Tests leak it on purpose.

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
    // exactly one customer's get_hair_cut().
    auto* shop = new FifoBarbershop(4);  // leaked on purpose
    auto tracker = std::make_shared<OverlapTracker>();
    auto gate = std::make_shared<std::counting_semaphore<>>(4);
    auto served = std::make_shared<std::atomic<int>>(0);

    auto cut_hair = [tracker] {
        // NB: runs on the user's barber thread — never throws, only records.
        tracker->enter("cutting");
        auto deadline = std::chrono::steady_clock::now() + 5s;
        while (tracker->current("being_cut") < 1 &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        if (tracker->current("being_cut") != 1) {
            tracker->violate("cut_hair ran without exactly one customer in "
                             "the chair");
        } else {
            std::this_thread::sleep_for(2ms);
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
    KOAN_ASSERT_EQ(served->load(), 10);
    KOAN_ASSERT_EQ(tracker->max_concurrent("being_cut"), 1);
}

KOAN_TEST(balk_when_full) {
    constexpr int n = 4;
    auto* shop = new FifoBarbershop(n);  // leaked on purpose
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
    eventually([cuts] { return cuts->current("cut") == 1; }, 5000ms,
               "the barber never started cutting");
    std::this_thread::sleep_for(300ms);  // let all n finish checking in

    auto balked = std::make_shared<std::atomic<int>>(0);  // 1=balk, 2=served
    assert_completes(
        [shop, balked] {
            balked->store(shop->customer_visit([] {}) ? 2 : 1);
        },
        2000ms, "customer n+1 (should balk immediately, not block)");
    KOAN_ASSERT_MSG(balked->load() == 1,
                    "customer n+1 should have balked with false");

    haircuts_may_finish->store(true);
    runner.join_all(10000ms);
    KOAN_ASSERT_EQ(served->load(), n);
}

KOAN_TEST(barber_sleeps_when_no_customers) {
    auto* shop = new FifoBarbershop(4);  // leaked on purpose
    auto cuts = std::make_shared<std::atomic<int>>(0);
    shop->start_barber([cuts] { cuts->fetch_add(1); });
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(cuts->load() == 0,
                    "the barber cut hair with no customer in the shop");
}

KOAN_TEST(stress_random_arrivals) {
    constexpr int n = 4;
    auto* shop = new FifoBarbershop(n);  // leaked on purpose
    auto tracker = std::make_shared<OverlapTracker>();
    auto served = std::make_shared<std::atomic<int>>(0);
    auto balked = std::make_shared<std::atomic<int>>(0);

    auto cut_hair = [tracker] {
        tracker->enter("cutting");
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
}

KOAN_TEST(served_in_arrival_order) {
    // Six customers arrive 25 ms apart — far wider than any registration
    // race — while the barber is not yet working. Once everyone is waiting,
    // the barber starts: haircuts must then happen in arrival order.
    // std::counting_semaphore makes NO ordering promise, so a koan-19-style
    // single shared semaphore may serve customers in any order at all.
    for (int trial = 0; trial < 3; ++trial) {
        auto* shop = new FifoBarbershop(8);  // leaked on purpose
        auto order = std::make_shared<EventLog>();
        ThreadRunner runner;

        for (int i = 0; i < 6; ++i) {
            runner.spawn(
                [shop, order, i] {
                    bool ok = shop->customer_visit(
                        [order, i] { order->record("c" + std::to_string(i)); });
                    KOAN_ASSERT_MSG(ok, "an in-capacity customer balked");
                },
                "customer-" + std::to_string(i));
            std::this_thread::sleep_for(25ms);
        }

        std::this_thread::sleep_for(150ms);  // all registered and waiting
        KOAN_ASSERT_MSG(order->events().empty(),
                        "trial " + std::to_string(trial) +
                            ": someone was served before the barber started");

        shop->start_barber([] {});
        runner.join_all(10000ms);
        std::vector<std::string> expected;
        for (int i = 0; i < 6; ++i) expected.push_back("c" + std::to_string(i));
        KOAN_ASSERT_MSG(order->events() == expected,
                        "trial " + std::to_string(trial) + ": service order " +
                            order->joined() +
                            " != arrival order — the barber must call "
                            "customers in FIFO order");
    }
}
