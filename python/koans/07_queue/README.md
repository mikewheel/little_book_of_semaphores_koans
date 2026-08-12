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

Edit `dancers.py`. Implement `DanceFloor` with:

- `leader_arrives()` — block until this leader has been matched with a
  follower, then return ("proceed to dance").
- `follower_arrives()` — block until this follower has been matched with
  a leader, then return.

## Traps worth savoring

The classic wrong answer deadlocks on the very first pair: each dancer
waits to be released *before* announcing their own arrival, so two
threads stand at the edge of the floor politely waiting for each other,
forever. If `pair_completes` times out, you have rediscovered koan 02's
central lesson.

## Python notes

Read a 0-initialized semaphore as a queue: `acquire()` means "get in
line", `release()` means "let one dancer out of that line". It is a
close cousin of a condition variable, minus the mutex handshake — and
unlike a `notify()`, a `release()` is never lost if nobody happens to be
waiting yet. That memory is what makes such a short solution possible.

Run: `./check python 07`
