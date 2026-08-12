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

Edit `rwlock.py`. Implement `ReadWriteLock` with four methods:

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

## Python notes

The stdlib has no reader-writer lock — people either vendor one (this
koan's class, essentially) or restructure around `queue`/immutability.
The count-plus-mutex technique you build here is the standard recipe
cited in every "why doesn't Python have an RWLock" thread.

Run: `./check python 11`
