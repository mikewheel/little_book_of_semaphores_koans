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

Edit `barrier.py`. Implement `Barrier(n)` with a single method `wait()`:
block until `n` threads in total have called it, then all return.

You will need more than a bare semaphore this time. The canonical
ingredient list: a counter of arrivals, a mutex (koan 03!) protecting that
counter, and a semaphore the early arrivals sleep on.

## Traps worth savoring

There are two famous wrong answers:

1. The `n`th thread signals the semaphore **once**, waking exactly one of
   the `n − 1` sleepers. The rest sleep forever. If your test run reports
   most-but-not-all threads passing, you've found it.
2. Waiting on the barrier semaphore *while still holding the mutex* —
   the sleepers hold the door shut against the very thread that would wake
   them. Total deadlock, caught by the watchdog.

The fix for #1 has a name worth learning — the **turnstile** — a
`wait()` immediately followed by a `signal()`, letting threads file
through one at a time.

Run: `./check python 05`
