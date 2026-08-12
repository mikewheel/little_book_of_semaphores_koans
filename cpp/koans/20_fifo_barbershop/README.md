# Koan 20 — FIFO Barbershop

*Adapted from* The Little Book of Semaphores, *§5.3 (CC BY-NC-SA 4.0).*

## The problem

Koan 19's shop has a quiet injustice: up to `n` customers can be waiting on
the same semaphore, and when the barber signals it, *any* of them may win
the chair. Semaphores make no fairness promise — a customer could in
principle wait forever while later arrivals keep getting lucky.

Fix it. Same shop, same rules — capacity `n`, balk with `false` when full,
barber sleeps when idle, haircuts pair 1:1 and finish fully — plus one new
guarantee:

- **Customers are served in arrival order**, where "arrival" is the moment
  `customer_visit` registers the customer (inside its mutual exclusion).

## Your task

Edit `fifo_barbershop.hpp`. Implement `FifoBarbershop(n)` with the same API
as koan 19:

- `start_barber(cut_hair)` — barber daemon; now it must call waiting
  customers strictly first-come-first-served.
- `customer_visit(get_hair_cut) -> bool` — balk immediately with `false`
  when full; otherwise wait until the barber calls *you specifically*, get
  your cut, return `true` when it is fully done.

The interesting move: a single shared "your turn" semaphore cannot express
"you specifically." Something per-customer has to appear.

## Traps worth savoring

- Reusing the koan-19 solution as-is: every waiter sits on one semaphore
  and wake order is whatever the implementation feels like.
  `std::counting_semaphore` documents no ordering at all — and even where a
  platform's futex queue happens to be FIFO today, nothing stops a spinning
  latecomer from stealing the token before the queued waiter wakes.
- Registering in the queue and signaling "a customer is ready" in the wrong
  order, or popping the queue outside the mutex — both let two threads
  disagree about who is first.

## Modern C++ notes (many ways to skin this cat)

- The per-waiter-channel queue you build here is the beating heart of every
  FIFO ticket lock, MCS/CLH queue lock, and futex wait-queue. Once you've
  written it by hand, those papers read themselves.
- The C++-specific spice is **object lifetime**: the customer creates a
  semaphore, the *barber* — another thread — signals it later. Stack
  allocation would be a use-after-free if the customer could ever leave
  early; the robust idiom is `std::shared_ptr<std::binary_semaphore>` in a
  `std::deque`, so the channel lives exactly as long as either side still
  holds it. (`std::binary_semaphore` can't sit in a deque by value anyway —
  it is neither copyable nor movable.)
- A `std::condition_variable` + ticket-number predicate is the other
  classic formulation (`cv.wait(lock, [&]{ return now_serving == my_ticket; })`
  with `notify_all`). It trades the per-waiter allocation for thundering-herd
  wakeups; the semaphore-queue version wakes exactly one thread.

Run: `./check cpp 20`
