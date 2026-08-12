# Koan 21 — Hilzer's Barbershop

*Adapted from* The Little Book of Semaphores, *§5.4 (CC BY-NC-SA 4.0).*

## The problem

Ralph Hilzer's upgrade of the barbershop (via William Stallings): a bigger
shop, a whole pipeline of stages, and several barbers working at once. The
shop has standing room, a sofa, several barber chairs, and one cash
register. Fire code caps the total heads inside at `capacity`.

A customer's trip: walk in (or balk if the shop is at capacity), stand
until a sofa seat frees up, sit on the sofa, wait to be called to a barber
chair, get the haircut, pay, and leave once a barber has taken the money.

The synchronization constraints:

1. Never more than `capacity` customers inside the shop.
2. Never more than `sofa_size` customers on the sofa.
3. Customers move from sofa to barber chair in sofa-seating order — the
   longest-seated customer is served first.
4. At most `n_barbers` haircuts happen at once, and `n_barbers` haircuts
   *can* happen at once (don't serialize the barbers).
5. A customer pays, and some barber accepts that payment before the
   customer's visit completes. There is one cash register: two
   `accept_payment` calls must never overlap.

## Your task

Edit `hilzers_barbershop.hpp`. The tests construct
`HilzersBarbershop(capacity, sofa_size, n_barbers, hooks)` where the
provided `HilzerHooks` struct carries six observation callbacks:
`enter_shop(cid)`, `sit_on_sofa(cid)`, `sit_in_chair(cid)`, `pay(cid)` for
customers, and `cut_hair(bid)`, `accept_payment(bid)` for barbers.
Implement:

- `customer_visit(cid) -> bool` — balk with an immediate `false` at
  capacity; otherwise drive the hook sequence `enter_shop → sit_on_sofa →
  sit_in_chair → pay` with all the waiting in between, returning `true`
  after your payment is accepted.
- `start_barbers()` — spawn `n_barbers` detached daemons (bid
  `0..n_barbers-1`), each looping: call the longest-waiting sofa-sitter to
  your chair, run `cut_hair(bid)`, then take a payment with
  `accept_payment(bid)`, register to yourself.

Contract fine print the tests rely on:

- A customer holds a shop slot from `enter_shop` until after the payment is
  accepted, and a sofa seat from `sit_on_sofa` until their `sit_in_chair`
  call *returns* — only then may the next customer sit down.
- `cut_hair(bid)` / `accept_payment(bid)` are called once per customer
  served / payment taken.

## Traps worth savoring

- One big lock around the whole pipeline "solves" every constraint and
  fails constraint 4: with the barbers serialized, two haircuts can never
  overlap and the concurrency test times out.
- A shared "your chair is ready" semaphore for all sofa-sitters loses
  constraint 3 the same way koan 19 lost FIFO — any waiter may win.
- Forgetting that a balked customer must undo nothing: if balking touches
  a queue or a seat count, the shop slowly loses capacity.

## Modern C++ notes (many ways to skin this cat)

- `HilzerHooks` is dependency injection through a plain struct of
  `std::function`s — the cheapest seam C++ offers for observing concurrent
  code from tests without friending your internals. The costs to know
  about: type erasure allocates, and calls through `std::function` won't
  inline. For hot paths you'd template the class on a hooks policy type
  instead (`template <class Hooks> class Shop`) and get both back.
- This koan is an argument for *composing small primitives* (a counter, a
  multiplex, a queue of channels, two rendezvouses) over one clever
  monolithic lock. Each stage can be reasoned about — and tested — alone.
  The monolithic alternative satisfies four constraints and quietly
  destroys the fifth (concurrency), which no amount of staring at the lock
  will reveal; only a liveness test does.
- Lifetime discipline recap from koan 20 applies doubly: per-customer
  channel objects cross threads, so they live in `std::shared_ptr`s; and
  the barbers are detached daemons parked on member semaphores, so the
  shop itself must never be destroyed while they sleep (the tests leak it
  deliberately — in production you'd want `jthread`s and a shutdown
  protocol instead).

Run: `./check cpp 21`
