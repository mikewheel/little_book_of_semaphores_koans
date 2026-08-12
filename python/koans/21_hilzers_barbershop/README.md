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

Edit `hilzers_barbershop.py`. The tests construct
`HilzersBarbershop(capacity, sofa_size, n_barbers, hooks)` where `hooks`
carries six observation callbacks: `enter_shop(cid)`, `sit_on_sofa(cid)`,
`sit_in_chair(cid)`, `pay(cid)` for customers, and `cut_hair(bid)`,
`accept_payment(bid)` for barbers. Implement:

- `customer_visit(cid) -> bool` — balk with an immediate `False` at
  capacity; otherwise drive the hook sequence `enter_shop → sit_on_sofa →
  sit_in_chair → pay` with all the waiting in between, returning `True`
  after your payment is accepted.
- `start_barbers()` — spawn `n_barbers` daemons (bid `0..n_barbers-1`),
  each looping: call the longest-waiting sofa-sitter to your chair, run
  `cut_hair(bid)`, then take a payment with `accept_payment(bid)`,
  register to yourself.

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

## Python notes

The hooks object is dependency injection for concurrency tests: the shop
under test calls into the suite at every stage boundary, which is what lets
the tests measure occupancy without guessing at your internals. When you
design concurrent components for real systems, leaving such seams (or
exposing stage-transition callbacks/events) is the difference between
testable and take-my-word-for-it. This solution is also a nice inventory
check: by this koan you have used the scoreboard, multiplex, FIFO queue of
private semaphores, and rendezvous patterns — all in one object.

Run: `./check python 21`
