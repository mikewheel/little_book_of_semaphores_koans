# Koan 11 — Readers-writers

*Adapted from* The Little Book of Semaphores, *§4.2 (CC BY-NC-SA 4.0).*

## The problem

A shared data structure — a cache, an index, a config blob — is read
constantly and written occasionally. Reads don't disturb each other, so
locking readers out of each other's way wastes parallelism. Writes are
different: a reader that overlaps a write can see the structure mid-
mutation, and two overlapping writes can corrupt it.

This is *categorical* mutual exclusion — exclusion between categories of
threads rather than between individual threads:

1. Any number of readers may be inside together.
2. A writer is inside **alone**: its presence excludes all readers *and*
   all other writers.

Both directions matter, and so does not over-constraining: a solution
that keeps writers safe by making readers take turns one at a time
satisfies the letter of rule 2 and flunks rule 1's spirit — and one of
the tests.

Starvation is explicitly out of scope: a writer may wait forever while
readers come and go. That injustice is koan 12's problem.

## Your task

Edit `rwlock.hpp`. Implement `ReadWriteLock` with four methods:

- `reader_enter()` — block while a writer is inside; otherwise proceed,
  even if other readers are already in.
- `reader_exit()` — leave; if you are the last reader out, whatever you
  do here matters to waiting writers.
- `writer_enter()` — block until nobody (reader or writer) is inside.
- `writer_exit()` — leave, letting someone else in.

The tests call these around their own critical sections; the lock never
sees what it protects.

## Traps worth savoring

- A single mutex shared by everyone is safe and wrong: readers get
  serialized. The concurrency test will name this failure.
- Forgetting that the *first* reader and the *last* reader have special
  jobs leads to writers entering over live readers, or writers waiting
  on a room that's been empty for ages.

## Modern C++ notes (many ways to skin this cat)

- `std::shared_mutex` + `std::shared_lock`/`std::unique_lock` *is* this
  koan, standardized (C++17). `lock_shared()` maps to `reader_enter`,
  `lock()` to `writer_enter`.
- What the standard pointedly does not specify: who wins when readers and
  writers are both waiting. Whether `std::shared_mutex` prefers readers,
  writers, or neither is up to the implementation (pthreads underneath,
  usually). Hand-rolling the lock — as here — is how you find out what
  policy you're actually getting, and the next two koans are exactly
  about choosing that policy on purpose.
- The counter-plus-mutex readers' protocol you write here is the same
  scheme inside most `shared_mutex` implementations; seeing it once makes
  their documentation's caveats ("writers may starve…") stop being
  mysterious.
- UB pitfall for real `shared_mutex` use: unlocking from a thread that
  doesn't hold the lock is undefined. This koan's semaphore formulation
  is thread-agnostic by design — the tests exploit that.

Run: `./check cpp 11`
