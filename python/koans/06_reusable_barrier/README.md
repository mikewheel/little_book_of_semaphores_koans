# Koan 06 — Reusable barrier

*Adapted from* The Little Book of Semaphores, *§3.7 (CC BY-NC-SA 4.0).*

## The problem

Koan 05's barrier opens once and stays open. But the common real-world
shape is a loop: `n` threads each compute a step, meet at the barrier,
and go around again — same barrier object, round after round. So the
barrier must lock itself behind each departing cohort.

For every round `r`:

- **Nobody leaves early.** No thread returns from its round-`r` `wait()`
  until all `n` threads have entered `wait()` that round.
- **Nobody laps the field.** No thread gets into round `r + 1`'s `wait()`
  until every thread has arrived at round `r`. A fast thread must never
  sneak around the loop and slip through with the previous cohort.

## Your task

Edit `reusable_barrier.py`. Implement `ReusableBarrier(n)` with:

- `phase1()` — arrival phase: block until all `n` threads have called it
  this round.
- `phase2()` — departure phase: block until all `n` threads have cleared
  the round, so that looping back to `phase1()` is safe.
- `wait()` — already written as `phase1()` then `phase2()`; keep it.
  Callers may also invoke the two phases separately with work in between
  (the book's Barrier-object API).

## Traps worth savoring

1. **The double open.** Check the arrival count *outside* the mutex and
   two threads can both observe "everyone is here" and both open the
   door. The surplus signal looks harmless — until a later round
   inherits it and someone leaks through, or everyone hangs.
2. **The lap.** Reuse a single turnstile and there is a moment after the
   cohort departs when it is still open. A quick thread loops around and
   strolls straight through, a full round ahead of a straggler. This is
   the book's storied "non-solution #2", and the tests hunt for it with
   jitter.

## Python notes

The stdlib ships this exact object as `threading.Barrier(n)` — reusable,
phase-counting, with an optional `action` callback and a broken-barrier
error model. Also handy: since Python 3.9, `Semaphore.release(n)` can
issue `n` tokens in one call. Build the barrier by hand here; afterwards
you get to use the stdlib one with a clear conscience.

Run: `./check python 06`
