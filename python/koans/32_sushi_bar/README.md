# Koan 32 — The sushi bar problem

*Adapted from* The Little Book of Semaphores, *§7.1 (CC BY-NC-SA 4.0).*

## The problem

A sushi bar has 5 seats and one social rule. If you arrive and a seat is
free — and the bar hasn't triggered the rule — you sit down immediately.
But the moment all 5 seats fill, the five of them are officially *dining
together*, and courtesy demands that nobody new squeeze in mid-party:
everyone who arrives from then on waits until the whole bar has emptied.
When the last of the party leaves, the entire waiting cohort (up to 5 of
them) takes its seats together — and if that refills the bar, the rule
re-arms for the next generation.

The rules your `SushiBar` must enforce:

1. Never more than `seats` (default 5) customers eating at once.
2. If a seat is free and the bar is **not** in must-wait mode, an
   arriving customer is seated immediately — no queueing for style
   points.
3. The bar enters must-wait mode the instant every seat is occupied, and
   stays in it until the bar is completely empty. Customers arriving
   during must-wait mode wait — *even while empty seats appear* as the
   party dwindles.
4. When the last incumbent leaves, the waiting cohort (up to `seats` of
   them) is seated together.

## Your task

Edit `sushi_bar.py`. Implement `SushiBar(seats=5)` with:

- `dine(eat)` — arrive, wait if the rules demand it, then run `eat()`
  while seated, then leave. Callable from many threads at once.

## Traps worth savoring

- The obvious `Semaphore(5)` seats people as soon as a seat frees. Rule 3
  says a freed seat during must-wait mode stays empty. There is a whole
  test dedicated to watching you fail this.
- The classic subtle bug (the book walks through it): a waiter who wakes
  up and then *re-acquires the lock to update the counters itself* races
  against brand-new arrivals. Newcomers can grab the lock first, see
  stale state, and take seats — you can end up with more diners than
  seats. The fix is a change of etiquette, not more locking: either the
  *departing* thread updates the waiters' state on their behalf, or the
  lock is *handed off* to a woken waiter without ever being released.

## Python notes

`threading.Semaphore.release(n)` seats a whole cohort in one call. The
two working etiquettes above are Reek's patterns — "I'll do it for you"
and "pass the baton" — and both are idiomatic in Python since
`threading.Semaphore` doesn't care which thread releases it. Hold the
scoreboard in plain ints under one `Lock`, and remember the moral the
book spells out: after you release the lock, the world may be rearranged
before anyone wakes up. Any invariant you need on wake-up must be
arranged *before* the release.

Run: `./check python 32`
