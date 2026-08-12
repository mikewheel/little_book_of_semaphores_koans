// Koan 21 — Hilzer's Barbershop (starter code). Edit this file only.
//
// Guarantees: at most `capacity` customers in the shop (late arrivals balk
// with false); at most `sofa_size` customers on the sofa; customers go
// from sofa to barber chair in sofa-seating order; at most `n_barbers`
// haircuts at once (and that many really can happen at once); each
// customer pays and has the payment accepted — at one cash register, one
// at a time — before their visit completes.
//
// Hook contract (each takes the customer id `cid` or barber id `bid`):
// - every served customer's thread calls, in order:
//   enter_shop(cid) → sit_on_sofa(cid) → sit_in_chair(cid) → pay(cid)
// - a customer occupies the shop from enter_shop(cid) until after payment
//   is accepted; a sofa seat from sit_on_sofa(cid) until sit_in_chair(cid)
//   has returned (only then may the next customer take the seat).
// - each barber thread calls cut_hair(bid) once per customer served and
//   accept_payment(bid) once per payment taken.
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>

#include "koans.hpp"

// The observation seams the tests inject. provided — do not modify.
struct HilzerHooks {
    std::function<void(int)> enter_shop = [](int) {};
    std::function<void(int)> sit_on_sofa = [](int) {};
    std::function<void(int)> sit_in_chair = [](int) {};
    std::function<void(int)> pay = [](int) {};
    std::function<void(int)> cut_hair = [](int) {};
    std::function<void(int)> accept_payment = [](int) {};
};

class HilzersBarbershop {
  public:
    HilzersBarbershop(int capacity, int sofa_size, int n_barbers,
                      HilzerHooks hooks)
        : capacity_(capacity),
          sofa_size_(sofa_size),
          n_barbers_(n_barbers),
          hooks_(std::move(hooks)) {
        // TODO: initialize the synchronization members you add below.
    }

    // Spawn n_barbers detached barber daemons (bid = 0 .. n_barbers-1).
    // Each barber loops forever: sleep until a customer is ready for a
    // chair, call that customer (oldest sofa-sitter first!), cut_hair(bid)
    // for exactly that customer, then take a payment: accept_payment(bid)
    // with the cash register to yourself.
    void start_barbers() {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        throw koans::NotImplemented{"HilzersBarbershop::start_barbers"};
    }

    // One customer's trip through the shop. Balk with an immediate false
    // if `capacity` customers are already inside. Otherwise walk the whole
    // pipeline — enter_shop, sit_on_sofa (waiting for a free seat),
    // sit_in_chair (waiting for a barber to call you, in sofa order), pay
    // — and return true once your payment has been accepted.
    bool customer_visit(int cid) {
        // ── YOUR CODE HERE ───────────────────────────────────────────────
        (void)cid;
        throw koans::NotImplemented{"HilzersBarbershop::customer_visit"};
    }

  private:
    int capacity_;
    int sofa_size_;
    int n_barbers_;
    HilzerHooks hooks_;
    // TODO: your synchronization members here.
};
