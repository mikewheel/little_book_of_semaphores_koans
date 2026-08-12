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

Edit `rendezvous.hpp`:

- add the semaphore members you need (plural — initial values matter);
- `run_a(a1, a2)` — call `a1()`, rendezvous with B, then call `a2()`;
- `run_b(b1, b2)` — call `b1()`, rendezvous with A, then call `b2()`.

Run: `./check cpp 02`

## Traps worth savoring

There is a classic wrong answer here that *deadlocks*: each thread waits
for the other before announcing itself. If the watchdog reports a timeout,
you've probably rediscovered it. There is also a subtly *inefficient* right
answer (wait first, then signal, forcing an extra context switch); the
tests can't catch that one, but the book discusses it.

## Modern C++ notes (many ways to skin this cat)

- The drill here is two `std::counting_semaphore<>`s (or two
  `std::binary_semaphore`s — a fine choice since each carries at most one
  token in this protocol).
- A two-party rendezvous is a barrier of size 2. In production C++20 you
  would reach for **`std::barrier<>`** (reusable) or **`std::latch`**
  (one-shot) rather than hand-rolling — you'll build a barrier yourself
  from semaphores in koans 05–06, then be allowed to appreciate the
  standard one.
- `std::binary_semaphore{0}` vs `std::counting_semaphore<>{0}`: binary is a
  type alias for `counting_semaphore<1>`; the max is a compile-time
  *ceiling*, and releasing above the ceiling is undefined behavior. When a
  semaphore logically never holds more than one token, saying so in the
  type documents the protocol.
