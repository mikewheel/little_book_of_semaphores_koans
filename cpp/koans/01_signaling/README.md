# Koan 01 — Signaling

*Adapted from* The Little Book of Semaphores, *§3.1 (CC BY-NC-SA 4.0).*

## The problem

Two threads share a semaphore. Thread A executes a statement we'll call
`a1`; thread B executes a statement we'll call `b1`. The scheduler is free
to run the threads in any interleaving — yet `b1` must never execute until
`a1` has completed.

This is **signaling**: the simplest possible use of a semaphore, enforcing a
"happens-before" edge between one event in one thread and one event in
another, without spinning, polling, or clocks.

## Your task

Edit `signaling.hpp`. Implement:

- the member(s) — create the semaphore(s) you need;
- `run_a(a1)` — thread A's body: call `a1()`, and arrange for B to proceed.
  A must never block waiting for B.
- `run_b(b1)` — thread B's body: call `b1()`, but only after `a1()` has
  finished. If B gets there first, it must *block* until A is done.

```sh
./check cpp 01
```

## Modern C++ notes (many ways to skin this cat)

- **`std::counting_semaphore` / `std::binary_semaphore`** (`<semaphore>`,
  C++20) are the direct translation of the book's semaphore. The book's
  `wait` is `acquire()`, `signal` is `release()`. Note the template
  parameter is a *ceiling*, not an initial value:
  `std::counting_semaphore<> sem{0};` starts at 0 with the default (large)
  ceiling. `std::binary_semaphore` is shorthand for a ceiling of 1.
- **`std::condition_variable` + a bool + `std::mutex`** is the pre-C++20
  workhorse for exactly this pattern. More ceremony, but it generalizes to
  arbitrary predicates. Worth knowing both.
- **`std::latch`** (C++20) is a one-shot countdown — `std::latch done{1};`
  with `count_down()`/`wait()` is arguably the *most* idiomatic modern
  answer for one-time signaling, because it cannot be misused twice.
- **`std::promise<void>`/`std::future<void>`** also expresses a one-shot
  happens-before edge, common in async codebases.

For this koan, use a semaphore — that's the pattern being drilled — but the
alternatives above are what you'll actually meet in real C++ code. A useful
habit from the book that C++ doesn't force on you: *name semaphores after
the fact they announce* (`a1_done`), not after who uses them.
