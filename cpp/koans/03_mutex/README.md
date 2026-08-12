# Koan 03 — Mutex

*Adapted from* The Little Book of Semaphores, *§3.4 (CC BY-NC-SA 4.0).*

## The problem

Two (or two hundred) threads each increment a shared counter. An increment
is secretly a read followed by a write, and the scheduler may interleave
the threads between those steps — so updates get lost. The classic fix is
**mutual exclusion**: wrap the update in a *critical section* that at most
one thread can occupy at a time.

Build that guard out of a semaphore. A semaphore used this way is a token
passed between threads: to enter the critical section you must hold the
token; leaving hands it back.

## Your task

Edit `mutex.hpp`. Implement a `Mutex` class backed by a
`std::counting_semaphore`:

- pick the initial value that means "one thread may enter";
- `acquire()` — block until the critical section is free, then claim it;
- `release()` — leave the critical section, admitting one waiter (if any).

The solution is symmetric — every thread runs the same two calls — and it
must work for *any* number of threads.

Run: `./check cpp 03`

## Modern C++ notes (many ways to skin this cat)

This koan is deliberately backwards from real practice, to make a point:

- In production you use **`std::mutex`** with an RAII guard —
  `std::lock_guard` (simple scope), `std::unique_lock` (when you need to
  unlock early or wait on a condition variable), or `std::scoped_lock`
  (multiple mutexes, deadlock-free ordering). Naked `lock()`/`unlock()`
  pairs are a code smell in C++ because any exception between them leaks
  the lock; RAII makes that impossible.
- A semaphore initialized to 1 *behaves* like a mutex but is not one to the
  standard: `std::mutex::unlock` must be called by the locking thread,
  whereas a semaphore may be released by *any* thread. That looseness is
  exactly what later koans exploit ("pass the baton"), and exactly why
  mutexes can be implemented more efficiently (e.g. priority-inheritance
  handoff on some platforms).
- For a lone counter, the real answer is neither: **`std::atomic<int>`**
  with `fetch_add`. The tests here split the read and write on purpose so
  that only genuine mutual exclusion passes.
- The test counter uses relaxed `std::atomic` loads/stores rather than a
  plain `int` so the broken interleaving is *observable* without being
  undefined behavior. Worth understanding why: a plain-int data race is UB
  even if it "works".
