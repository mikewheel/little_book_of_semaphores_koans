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

Edit `handmade_semaphore.hpp`. Implement `HandmadeSemaphore(int value = 0)`
(nonnegative initial value; the tests never pass less) with:

- `acquire()` — take a permit, blocking until one is available.
- `release()` — bank a permit; wake exactly one waiter if any.

House rule: **only** `std::mutex` and `std::condition_variable` — the
starter's includes are exactly the allowed toolbox. Adding
`#include <semaphore>` here is an honor-system foul (and would defeat
the koan).

## Traps worth savoring

- Waiting on the condition variable without holding the lock, or checking
  the predicate once with `if` instead of re-checking in a loop — the two
  classic condition-variable misuses. (The standard *requires* the loop:
  spurious wakeups are allowed to happen.)
- The plausible-but-subtly-wrong version: track only `value`, wait in
  `while (value <= 0)`, decrement after waking. It passes most tests —
  see "what the tests can't see" below for the crack in it.

## What the tests can't see

Property 3 is the hard one to test from outside. The suite stages a
releaser that immediately turns around and re-acquires while a waiter is
parked; the `while (value <= 0)` version tends to lose that race (the
releaser snatches its own permit back and the sleeper stays parked), and
the test catches it. But "tends to" is the honest phrase: a scheduler can
mask the theft, and no black-box test can prove Property 3 the way the
book's argument can. If your implementation passes the suite but you
never needed a second counter, read the last hint anyway — the reasoning
is the actual lesson of this koan. (The book also concedes a footnote-
sized exception: a perfectly timed spurious wakeup can defeat even the
correct version.)

## Modern C++ notes (many ways to skin this cat)

- This is THE koan for condition-variable discipline. `wait(lock)` must
  hold the lock; the predicate goes in a loop (or the predicate overload
  `wait(lock, pred)`, which is the loop) because the standard explicitly
  permits **spurious wakeups** — that permission is why predicate loops
  are mandated style, not paranoia.
- `notify_one()` under the lock vs. after unlocking is a real trade: under
  the lock is a clean hand-off (the woken thread finds the state exactly
  as the signaler left it, at the cost of the waker briefly blocking the
  wakee — "hurry up and wait"); after unlocking can save a context-switch
  bounce but lets a third thread slip in between. The book's Property 3
  machinery (`wakeups`) is precisely a hand-off made explicit.
- Why does `std::counting_semaphore` exist if this class works? The
  standard one can use the platform's futex fast path: an uncontended
  `acquire()` is a single atomic op, no mutex, no syscall. Your handmade
  version takes a full lock round-trip every time. After this koan,
  reading libstdc++'s `<semaphore>` implementation is genuinely fun — you
  will recognize every move.

Run: `./check cpp 41`
