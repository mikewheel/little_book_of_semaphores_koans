# Koan 08 — Exclusive queue

*Adapted from* The Little Book of Semaphores, *§3.8 (CC BY-NC-SA 4.0).*

## The problem

Koan 07 paired the dancers up but let any number of pairs pile onto the
floor at once. Now the floor is small: it fits **one pair at a time**,
and each dancer actually dances — your methods receive a `dance`
callback to invoke at the right moment.

Constraints:

1. Dancers still proceed only in leader/follower pairs; a lone dancer
   waits (without dancing) until a partner of the opposite kind arrives.
2. **At most one pair is on the floor at a time.** A leader's `dance()`
   and its partner's `dance()` overlap in time — they dance *together* —
   and no other dancer's `dance()` may overlap either of them.
3. **A leader does not leave first.** `leader_dances(dance)` must not
   return until its partner's `dance()` has completed. (The follower may
   leave without waiting for the leader — the courtesy is one-way, which
   matches the book's solution.)

## Your task

Edit `exclusive_dancers.py`. Implement `ExclusiveDanceFloor` with:

- `leader_dances(dance)` — block until paired with a follower, invoke
  `dance()` while the pair has the floor to itself, and return only
  after the partner's dance has completed.
- `follower_dances(dance)` — block until paired with a leader, then
  invoke `dance()` while the pair has the floor to itself.

## Traps worth savoring

- **The mosh pit.** Reusing your koan 07 solution and simply calling
  `dance()` after the pairing point pairs everyone correctly — and lets
  every pair dance at once. The tests count floor occupancy.
- **The lock nobody can return.** Guarding the floor with a lock that
  the same dancer both takes and gives back doesn't fit this timeline:
  the dancer who claims the floor when the pair forms is not the one who
  knows when the pair is finished. Sit with that sentence — it is the
  whole koan.

## Python notes

Python's plain `threading.Lock` has no owner: any thread may release a
lock that a different thread acquired (`RLock` is the one that checks).
The book exploits exactly this property of its `Semaphore(1)`-as-mutex.
So Python would let you write this koan with a `Lock` — but spelling the
guard as a semaphore makes the unusual hand-off legible instead of
looking like a bug.

Run: `./check python 08`
