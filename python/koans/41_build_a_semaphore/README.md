# Koan 41 — Build a semaphore

*Adapted from* The Little Book of Semaphores, *§9.2 (CC BY-NC-SA 4.0).*

## The problem

Forty koans of spending semaphores; now mint your own. The most common
synchronization tools in Pthreads-style programming are mutexes and
condition variables — so build the book's semaphore out of exactly those
two ingredients, and rediscover why one extra counter stands between a
plausible implementation and a correct one.

Your semaphore must satisfy the book's three properties:

1. When a thread calls `acquire()` while the semaphore is exhausted, it
   blocks; otherwise it proceeds (imagine the value counting how many
   permits remain).
2. `release()` **banks** a permit: a release with nobody waiting is not
   lost — the next `acquire()` sails through. (This is the deep
   difference from a condition-variable notify, which evaporates if
   nobody is waiting.)
3. When `release()` wakes the waiters, **a thread that was actually
   waiting gets in**. A fresh thread calling `acquire()` at just the
   wrong moment must not snatch the wakeup out from under a sleeper.

## Your task

Edit `handmade_semaphore.py`. Implement `HandmadeSemaphore(value=0)`
(nonnegative initial value; the starter already validates it) with:

- `acquire()` — take a permit, blocking until one is available.
- `release()` — bank a permit; wake exactly one waiter if any.

House rule: **only** `threading.Lock` / `threading.Condition` (a plain
mutex and condition variable). The stdlib's ready-made semaphore classes
are banned, and one test reads this module's source to enforce that.

## Traps worth savoring

- Waiting on the condition *without* holding its lock, or re-checking
  state after `wait()` returns as if nothing could have changed in
  between — the two classic condition-variable misuses.
- The plausible-but-subtly-wrong version: track only `value`, wait in
  `while value <= 0`, decrement after waking. It passes most tests —
  see "what the tests can't see" below for the crack in it.

## What the tests can't see

Property 3 is the hard one to test from outside. The suite stages a
releaser that immediately turns around and re-acquires while a waiter is
parked; the `while value <= 0` version usually loses that race (the
releaser snatches its own permit back and the sleeper stays parked), and
the test catches it. But "usually" is the honest word: a scheduler can
mask the theft, and no black-box test can prove Property 3 the way the
book's argument can. If your implementation passes the suite but you
never needed a second counter, read the last hint anyway — the reasoning
is the actual lesson of this koan. (The book also concedes a footnote-
sized exception: a perfectly timed spurious wakeup can defeat even the
correct version.)

## Python notes

`threading.Condition` bundles the lock and the waiting room:
`with cond:` takes the lock, `cond.wait()` releases it while sleeping
and reacquires before returning, `cond.notify()` wakes one sleeper.
Python has no do-while, so the book's post-test loop needs restating
with `while True:` — and the GIL is no substitute for the lock: `wait()`
must still be called with the condition held.

Run: `./check python 41`
