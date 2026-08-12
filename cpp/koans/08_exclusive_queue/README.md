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

Edit `exclusive_dancers.hpp`. Implement `ExclusiveDanceFloor` with:

- `leader_dances(dance)` — block until paired with a follower, invoke
  `dance()` while the pair has the floor to itself, and return only
  after the partner's dance has completed.
- `follower_dances(dance)` — block until paired with a leader, then
  invoke `dance()` while the pair has the floor to itself.

Run: `./check cpp 08`

## Traps worth savoring

- **The mosh pit.** Reusing your koan 07 solution and simply calling
  `dance()` after the pairing point pairs everyone correctly — and lets
  every pair dance at once. The tests count floor occupancy.
- **The lock nobody can return.** Guarding the floor with a lock that
  the same dancer both takes and gives back doesn't fit this timeline:
  the dancer who claims the floor when the pair forms is not the one who
  knows when the pair is finished. Sit with that sentence — it is the
  whole koan.

## Modern C++ notes (many ways to skin this cat)

- **The big one, in lights: `std::mutex` may only be unlocked by the
  thread that locked it.** Unlocking from any other thread is undefined
  behavior — the standard's mutex requirements bake ownership into the
  type ([thread.mutex.requirements]), and so do POSIX mutexes underneath.
  The canonical solution to this puzzle passes the critical-section
  guard *between* threads: whoever claims the floor when the pair forms
  is not who releases it when the pair leaves. With a `std::mutex` (or
  `std::lock_guard`/`std::unique_lock`) that protocol is simply illegal.
  **Semaphores have no owner**: `std::binary_semaphore{1}` used as a
  mutex may be acquired in one thread and released in another, fully
  defined. This koan is precisely why the book's "mutex" is a
  `Semaphore(1)` — the exercise where lock ownership stops being a
  pedantic footnote.
- If you want every unlock to stay on the locking thread, the
  alternative is a `std::condition_variable` + state-machine
  formulation (counters plus an "on the floor" flag, with predicate
  loops). It works, is bigger, and is a good second implementation to
  compare against.
- The callbacks arrive as `const std::function<void()>&` — call them,
  don't copy them; and think about exception safety only as far as the
  koan demands (the tests don't throw from `dance`).
