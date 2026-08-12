# Koan 13 — Writer-priority readers-writers

*Adapted from* The Little Book of Semaphores, *§4.2 (CC BY-NC-SA 4.0).*

## The problem

Koan 12 stopped writers starving, but it hands out the room one turn at a
time: after a writer finishes, whoever queued next — reader or writer —
goes in. Sometimes that's too even-handed. If writers carry time-critical
updates, every reader that slips in between two pending writes is a
reader served stale data.

So flip the bias all the way. Same safety rules as ever:

1. Readers may share the room.
2. A writer is inside alone.

New priority rule:

3. Once any writer is waiting or writing, **no new reader may enter**
   until every currently-queued writer has finished. Writers hand the
   room directly to each other; readers wait for the whole convoy to
   drain.

Concurrency between readers must survive: with no writer around, readers
still share freely. And be honest about the cost — under a steady stream
of writers, readers now starve. That's the trade you're choosing.

## Your task

Edit `rwlock_writer_priority.hpp`. Implement
`WriterPriorityReadWriteLock` with the same four methods:
`reader_enter()`, `reader_exit()`, `writer_enter()`, `writer_exit()` —
plus the priority rule above.

## Traps worth savoring

- Koan 12's turnstile is *too fair* here: it wakes exactly one waiter per
  writer exit, so a reader can slither in between two queued writers.
  The ordering test stages precisely that ambush.
- Symmetric to koan 11's lesson: now it's the *writers* who need a
  first-in/last-out collective claim on something.

## Modern C++ notes (many ways to skin this cat)

- This is the "write-preferring rwlock" policy — what
  `pthread_rwlock` calls `PREFER_WRITER` and what several database lock
  managers do unconditionally. `std::shared_mutex` may or may not behave
  this way; the standard is silent, which is exactly why building the
  policy explicitly is worth an evening.
- The composition has a name worth remembering: **two lightswitches**.
  Readers collectively hold `no_writers`; writers collectively hold
  `no_readers`. Each category's first-in/last-out claim is one
  lightswitch; the whole lock is just the two of them crossed.
- The real-world bill: in read-heavy services a write-preferring lock
  turns bursts of writes into visible read-latency spikes (the readers
  queue for the whole convoy). If reads have SLOs, people often prefer
  stale-tolerant designs — RCU, epoch-based reclamation, or snapshots —
  over strict writer priority.
- UB reminder if you swap in `std::shared_mutex` later: releasing a lock
  from a thread that never took it is undefined; the semaphore version
  here is deliberately thread-agnostic, and the tests use that freedom.

Run: `./check cpp 13`
