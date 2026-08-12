# Koan 05 — Barrier

*Adapted from* The Little Book of Semaphores, *§3.6 (CC BY-NC-SA 4.0).*

## The problem

The rendezvous (koan 02) only meets two threads. Generalize it: `n` threads
each reach a synchronization point, and **nobody may pass it until all `n`
have arrived**. The first `n − 1` arrivals block; the `n`th arrival releases
everyone.

This koan is a one-shot barrier: it only has to work once. (Making it
reusable is koan 06, and that difference is where the real teeth are.)

## Your task

Edit `barrier.hpp`. Implement `Barrier` (constructed with `n`) with a
single method `wait()`: block until `n` threads in total have called it,
then all return.

You will need more than a bare semaphore this time. The canonical
ingredient list: a counter of arrivals, a mutex protecting it, and a
semaphore the early arrivals sleep on.

Run: `./check cpp 05`

## Traps worth savoring

There are two famous wrong answers:

1. The `n`th thread releases the semaphore **once**, waking exactly one of
   the `n − 1` sleepers. The rest sleep forever.
2. Sleeping on the barrier semaphore *while holding the mutex* — the
   sleepers hold the door shut against the very thread that would wake
   them. Total deadlock, caught by the watchdog.

The fix for #1 has a name worth learning — the **turnstile**: an
`acquire()` immediately followed by a `release()`, letting threads file
through one at a time. (Or release `n` tokens at once — see the hints.)

## Modern C++ notes (many ways to skin this cat)

- C++20 ships `std::latch` (one-shot countdown, exactly this koan) and
  `std::barrier<>` (reusable, phase-aware, optional completion function —
  koan 06's problem). In real code use those. Here you're building the
  thing so the standard components stop being magic.
- For the arrival counter you'd reach for `std::mutex` +
  `std::lock_guard` in practice; using your koan-03 semaphore-as-mutex is
  equally valid here and keeps you in the book's vocabulary. Either is
  accepted by the tests.
- `release(n)` exists: `std::counting_semaphore::release` takes an update
  count. The "preloaded turnstile" solution is a one-liner with it.
