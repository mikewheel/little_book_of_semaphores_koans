# Koan 04 — Multiplex

*Adapted from* The Little Book of Semaphores, *§3.5 (CC BY-NC-SA 4.0).*

## The problem

Generalize the mutex: instead of admitting exactly one thread to the
critical section, admit **up to `n` at a time**. Thread `n + 1` must wait
until someone inside leaves. This is a **multiplex** — the bouncer at a
club with a fire-code capacity: people stream in until the room is full,
then it's one-out-one-in.

Two properties, and the tests check both:

- *Safety*: never more than `n` threads inside.
- *Concurrency*: `n` threads genuinely can be inside together. (A plain
  mutex satisfies safety and fails this — don't over-constrain.)

## Your task

Edit `multiplex.py`. Implement `Multiplex(n)` with `enter()` and `exit()`.
If you solved koan 03, this one is a single realization away.

Run: `./check python 04`

## Python notes

`threading.BoundedSemaphore` is the stdlib's guard-railed variant: it
raises if you `release()` more times than you acquired — nice insurance
against a miscounted multiplex. Real-world siblings of this pattern:
connection pools, rate limiters, `concurrent.futures` worker caps.
