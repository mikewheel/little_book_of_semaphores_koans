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

Edit `savages.py`. The tests construct `Village(m, pot)` where `pot` is an
instrumented object providing `put_servings(m)` and `get_serving()`; it
records a violation whenever it is refilled non-empty or drawn from empty.
The pot is **not** thread-safe — your code must never let two pot calls
overlap. Implement:

- `start_cook()` — spawn the cook as a daemon thread. It sleeps until some
  diner reports the pot empty, calls `pot.put_servings(self.m)`, announces
  the refill, and loops.
- `dine()` — called concurrently from many diner threads. Take exactly one
  serving with `pot.get_serving()`; if the pot is empty, wake the cook and
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

## Python notes

`threading.Semaphore` hides its internal counter on purpose — there is no
portable "peek" (and code that reads `sem._value` is lying to itself). When
an algorithm needs to *inspect* the count, keep the count yourself in a
plain integer guarded by a lock; that is the scoreboard idiom this koan
exists to teach. The cook thread is a daemon: it blocks forever waiting for
work, and daemon threads are how Python lets such servants die with the
process.

Run: `./check python 18`
