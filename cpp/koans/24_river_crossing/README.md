# Koan 24 — River crossing

*Adapted from* The Little Book of Semaphores, *§5.7 (CC BY-NC-SA 4.0).*

## The problem

A rowboat near Redmond ferries two mutually suspicious populations across
a river: Linux hackers and Microsoft serfs. House rules:

- The boat departs with **exactly four** aboard — never more, never fewer.
- A 3-to-1 split is forbidden in either direction (one hacker among three
  serfs would have a bad time, and vice versa). Legal crews: four hackers,
  four serfs, or two of each.
- Each passenger calls `board(...)` as they get in. All four `board` calls
  of one crew must happen before any `board` of the next crew — no seat
  squatting across boatloads.
- After all four have boarded, **exactly one** of them calls
  `row_boat(...)`. Any of the four may row; precisely one must.
- Traffic is one-directional; the boat teleports back for free.

A thread arriving when no legal crew can be completed simply waits —
possibly forever, if the right mix never shows up.

## Your task

Edit `river.hpp`. `Boat` is constructed with `BoatHooks` (`board`,
`row_boat`). Implement:

- `hacker_arrives()` — block until this thread is part of a legal crew of
  four, call `hooks_.board("hacker")`, and return once the boatload has
  sailed. If this thread is the one rowing, call
  `hooks_.row_boat("hacker")` after all four boards.
- `serf_arrives()` — the same with kind `"serf"`.

Run: `./check cpp 24`

## Traps worth savoring

- Counting arrivals but letting the *next* crew start boarding while the
  current one is still climbing in: with a slow `board`, boatload
  boundaries smear and the tests' block-of-four partition catches it.
- Zero rowers or four rowers per boat. "Exactly one of the four calls
  `row_boat`" needs each thread to know a little about the role it played
  in completing the crew.
- Waking two hackers and two serfs when the state was actually four
  hackers and one serf — check your conditions with a cold eye; the tests
  park a 3+1 mix on the dock and watch for 400 ms.

## Modern C++ notes (many ways to skin this cat)

- "Am I the captain?" is per-*call* state. A plain stack local is exactly
  right: each thread's invocation gets its own. Reaching for the
  `thread_local` keyword here is a category error — that's per-thread
  *static* storage, which outlives the call, is shared across every
  invocation on that thread, and would be flat wrong if a thread ever rode
  twice. Locals are the humblest and most correct thread-private storage.
- The crew barrier is a rendezvous of 4: reuse your koan-06 barrier or
  `std::barrier<>`. `std::counting_semaphore::release(4)` batch-frees a
  whole crew in one call.
- The completer keeps the dock mutex until the boat sails, then a possibly
  different thread releases it — so a `std::binary_semaphore` (no owner)
  is the right "mutex", not `std::mutex` (owner-only unlock is the rule;
  breaking it is UB).
- Fairness caveat worth knowing: this design can starve. A serf eligible
  only via the 2+2 crew can wait unboundedly while all-hacker crews keep
  forming. The book accepts this; production designs would add queueing
  discipline (tickets, FIFO condition variables) if it mattered.
