# Koan 18 — Dining Savages

*Adapted from* The Little Book of Semaphores, *§5.1 (CC BY-NC-SA 4.0).*

## The problem

A village eats from one communal pot that holds up to `m` servings. Diners
help themselves, one serving at a time — but only while there is something
in the pot. The first diner to find it empty wakes the village cook, then
waits; the cook refills the pot with `m` fresh servings and goes back to
sleep. (The book frames this with cartoon "savages" and stewed missionary,
a wink at the dining philosophers; we keep the math and lose the stew.)

The synchronization constraints:

- `get_serving()` may only be called when the pot is non-empty.
- `put_servings(m)` may only be called when the pot is completely empty.
- The cook does nothing until woken — no refilling on a timer, no topping
  up a half-full pot.

Because refills happen only on demand, serving `k` meals from an
initially-empty pot takes **exactly** `ceil(k / m)` refills — and the tests
count.

## Your task

Edit `savages.hpp`. The tests construct `Village(m, pot)` where `pot`
implements the provided `Pot` interface with instrumentation: it records a
violation whenever it is refilled non-empty or drawn from empty. The pot is
**not** thread-safe — your code must never let two pot calls overlap.
Implement:

- `start_cook()` — spawn the cook as a detached daemon thread. It sleeps
  until some diner reports the pot empty, calls `pot.put_servings(m_)`,
  announces the refill, and loops.
- `dine()` — called concurrently from many diner threads. Take exactly one
  serving with `pot_.get_serving()`; if the pot is empty, wake the cook and
  wait until the refill is announced before taking it.

## Traps worth savoring

- The producer–consumer reflex — a semaphore whose value *is* the number of
  servings — runs aground here: a thread cannot ask a semaphore "would I
  block?" before acquiring it, so nobody can tell that the pot is empty and
  the cook never gets woken (or gets woken at the wrong times). The failure
  mode is diners parked forever on an empty pot, or refill counts that
  drift from the exact `ceil(k/m)`.
- Letting diners touch the pot outside your exclusion, or waking the cook
  without also blocking new diners, shows up as pot violations: a refill
  landing on a non-empty pot, or two diners splitting the same serving.

## Modern C++ notes (many ways to skin this cat)

- Squint and this is a `std::condition_variable` predicate loop in
  disguise: the diner's "empty → wake cook → wait for full" is
  `cv.wait(lock, [&]{ return servings > 0; })` plus a `notify` aimed at the
  cook. The semaphore version the book teaches makes the *handoff* explicit
  (who wakes whom, and exactly once); the CV version makes the *predicate*
  explicit and tolerates spurious wakeups by construction. Both are
  idiomatic — know how to write both.
- The cook here is a detached daemon parked on a semaphore, which is why
  the tests deliberately leak the `Village`: destroying a semaphore with a
  waiter blocked on it is undefined behavior. In production you'd reach for
  `std::jthread` and a `std::stop_token` so the cook can be asked to quit —
  but note a `stop_token` cannot interrupt a plain `semaphore::acquire()`;
  you would need `try_acquire_for` in a loop, or a CV with
  `condition_variable_any::wait(lock, stoken, pred)`, which is precisely
  why `jthread` pairs so well with CVs.
- `std::counting_semaphore<>`'s template parameter is a compile-time
  ceiling, not a value — the runtime initial count goes to the constructor.
  There is no way to read a semaphore's count, and that is not an
  oversight; this koan's scoreboard is the sanctioned workaround.

Run: `./check cpp 18`
