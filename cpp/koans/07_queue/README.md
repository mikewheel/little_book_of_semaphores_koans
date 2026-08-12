# Koan 07 — Queue

*Adapted from* The Little Book of Semaphores, *§3.8 (CC BY-NC-SA 4.0).*

## The problem

So far your semaphores have started at 1 (a lock) or `n` (a room with
capacity). Start one at **0** and it becomes something else entirely: a
*queue* of parked threads, where every `release()` lets exactly one of
them out.

The setting is a ballroom. Two kinds of dancers — leaders and followers —
arrive at the floor:

- A leader who arrives when no follower is available must wait; likewise
  a follower with no leader.
- When both kinds are present, one leader and one follower proceed to
  the floor together — dancers advance strictly in matched pairs, so the
  number of leaders that have ever proceeded always equals the number of
  followers.
- Nothing more. In particular there is **no mutual exclusion**: any
  number of matched pairs may be out on the floor at once. This koan is
  purely about pairing (its exclusive sibling is koan 08).

## Your task

Edit `dancers.hpp`. Implement `DanceFloor` with:

- `leader_arrives()` — block until this leader has been matched with a
  follower, then return ("proceed to dance").
- `follower_arrives()` — block until this follower has been matched with
  a leader, then return.

Run: `./check cpp 07`

## Traps worth savoring

The classic wrong answer deadlocks on the very first pair: each dancer
waits to be released *before* announcing their own arrival, so two
threads stand at the edge of the floor politely waiting for each other,
forever. If `pair_completes` times out, you have rediscovered koan 02's
central lesson.

## Modern C++ notes (many ways to skin this cat)

- This is the **semaphore-as-queue idiom**: a `std::counting_semaphore<>`
  constructed at 0, where `acquire()` means "get in line" and `release()`
  means "let one out". Each dancer releases the opposite kind's semaphore
  exactly once and acquires its own exactly once, so tokens conserve the
  pairing invariant with no shared counters at all — which is why,
  unusually, **no mutex is needed anywhere** in this koan.
- The `std::condition_variable` formulation needs more machinery: a
  mutex, two waiting-counts, and predicate loops — because a `notify()`
  with nobody waiting evaporates, whereas a `release()` is banked. When
  you find yourself adding a counter next to a CV just to remember missed
  notifies, you have reinvented a semaphore.
- `std::binary_semaphore` would be wrong here: several dancers of one
  kind can be owed passage at once, so the count must be able to grow.
  Exceeding a semaphore's `least_max_value` ceiling is undefined
  behavior — another reason the unbounded-ish default
  `counting_semaphore<>` is the right tool.
