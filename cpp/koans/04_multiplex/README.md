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

Edit `multiplex.hpp`. Implement `Multiplex` (constructed with `n`) with
`enter()` and `exit()`. If you solved koan 03, this one is a single
realization away.

Run: `./check cpp 04`

## Modern C++ notes (many ways to skin this cat)

- This is the one koan where the C++20 primitive *is* the pattern:
  `std::counting_semaphore` was standardized largely for
  multiplex-shaped problems (pools, throttles, bounded pipelines).
- Wrinkle: the template parameter (`least_max_value`) is a compile-time
  ceiling, but our `n` arrives at runtime. Idiomatic answer: use the
  default ceiling (`std::counting_semaphore<>`) and pass the runtime `n`
  to the *constructor*. The type-level number is a promise about the
  maximum you'll ever hold, not the count.
- In real code, consider an RAII gate so `exit()` can't be forgotten on an
  exception path: a small `struct Pass { Multiplex& m; ~Pass(){m.exit();} }`
  or a `std::unique_ptr` with a custom deleter. The koan keeps explicit
  `enter`/`exit` because that's the vocabulary the later puzzles build on.
- There's no bounded/checked variant in the standard library (Python has
  `BoundedSemaphore`); releasing more than you acquired past the ceiling is
  UB. Discipline (or a debug assert) is on you.
