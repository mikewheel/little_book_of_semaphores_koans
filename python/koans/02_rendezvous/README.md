# Koan 02 — Rendezvous

*Adapted from* The Little Book of Semaphores, *§3.3 (CC BY-NC-SA 4.0).*

## The problem

Signaling (koan 01) is one-directional. Make it symmetric: two threads must
*meet* at a point in their execution, and neither may continue past that
point until the other has reached it.

Thread A runs statement `a1`, then statement `a2`. Thread B runs `b1`, then
`b2`. Enforce both of these orderings:

- `a1` happens before `b2`
- `b1` happens before `a2`

…and nothing more. Do **not** constrain the order of `a1` versus `b1`; over
many runs either may finish first. Whichever thread reaches the meeting
point first blocks until the other arrives — a **rendezvous**.

## Your task

Edit `rendezvous.py`. Implement:

- `Rendezvous.__init__` — create the semaphores you need (plural — and the
  initial values matter).
- `run_a(a1, a2)` — call `a1()`, rendezvous with B, then call `a2()`.
- `run_b(b1, b2)` — call `b1()`, rendezvous with A, then call `b2()`.

## Traps worth savoring

There is a classic wrong answer here that *deadlocks*: each thread waits for
the other before announcing itself. If your tests time out, you've probably
rediscovered it — congratulations, and reread the definition of deadlock.
There is also a subtly *inefficient* right answer (wait first, then signal,
forcing an extra context switch). The tests can't catch that one; the book
discusses it.

Run: `./check python 02`
