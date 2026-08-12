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

Edit `rwlock_writer_priority.py`. Implement `WriterPriorityReadWriteLock`
with the same four methods: `reader_enter()`, `reader_exit()`,
`writer_enter()`, `writer_exit()` — plus the priority rule above.

## Traps worth savoring

- Koan 12's turnstile is *too fair* here: it wakes exactly one waiter per
  writer exit, so a reader can slither in between two queued writers.
  The ordering test stages precisely that ambush.
- Symmetric to koan 11's lesson: now it's the *writers* who need a
  first-in/last-out collective claim on something.

## Python notes

This is the "write-preferring rwlock" you'll find as a knob (or a fixed
choice) in database engines and some RWLock recipes. Nothing in the
stdlib implements it; you're about to know why the recipes all have two
counters in them.

Run: `./check python 13`
