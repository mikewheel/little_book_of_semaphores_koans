# Koan 19 — Barbershop

*Adapted from* The Little Book of Semaphores, *§5.2 (CC BY-NC-SA 4.0).*

## The problem

Dijkstra's sleeping barber. A shop holds at most `n` customers — the
waiting seats plus the barber's chair. One barber works there:

- With nobody around, the barber dozes in his own chair.
- An arriving customer who finds the shop full turns around and leaves —
  a **balk** — immediately, without waiting.
- Otherwise the customer takes a seat; if the barber is asleep, the arrival
  wakes him.
- Each haircut involves both parties at once: while the barber runs
  `cut_hair()`, exactly one customer must be running `get_hair_cut()`, and
  the barber may not start on anyone else until that customer's cut is
  completely finished.

Nothing here promises first-come-first-served — any waiting customer may be
called next. (Koan 20 adds that guarantee.)

## Your task

Edit `barbershop.hpp`. Implement `Barbershop(n)` with:

- `start_barber(cut_hair)` — spawn the barber as a detached daemon thread:
  loop forever, sleeping until a customer is present, then run `cut_hair()`
  paired 1:1 with that one customer's `get_hair_cut()`, finishing this
  customer fully before taking the next.
- `customer_visit(get_hair_cut) -> bool` — return `false` immediately (no
  blocking) if `n` customers are already in the shop; otherwise wait for
  your turn, run `get_hair_cut()` concurrently with the barber's
  `cut_hair()`, and return `true` only once both sides of the cut are done.

## Traps worth savoring

- Checking "is the shop full?" anywhere but inside your mutual exclusion
  invites two customers to squeeze through the same last seat.
- The subtle classic: signaling the customer to take the chair and then
  cutting, *without* waiting to hear the customer is done. The barber
  finishes `cut_hair()`, loops around, and starts on the next head while
  the previous customer is still in a chair — the tests watch for exactly
  that overlap.

## Modern C++ notes (many ways to skin this cat)

- Balk-by-`bool` vs balk-by-exception: the book's `balk()` never returns.
  In C++ a `bool` (or `std::optional`/`std::expected` for richer results)
  is usually kinder than an exception for an *expected* outcome — and
  exceptions thrown across a callback boundary in concurrent code are a
  good way to lose track of who unwinds what.
- The `customers` counter must be decremented on **every** exit path. Here
  there is only one, but the moment `get_hair_cut()` can throw, you want
  RAII: a tiny scope guard —
  `struct ScopeExit { std::function<void()> f; ~ScopeExit(){ f(); } };` —
  or `std::experimental::scope_exit` where available, so the seat is given
  back even during unwinding.
- One `release()` on a semaphore wakes one waiter, but *which* one is
  unspecified — that looseness is exactly why this shop is not FIFO. Herd
  effects (many waiters, one token) are tame here; they become the whole
  problem in koan 20.

Run: `./check cpp 19`
