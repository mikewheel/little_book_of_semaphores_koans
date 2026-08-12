# Koan 34 — Extended child care

*Adapted from* The Little Book of Semaphores, *§7.2 (CC BY-NC-SA 4.0).*

## The problem

Koan 33's center is safe but rude. Picture 4 children and 2 adults inside,
and one adult heading for the door. She cannot leave yet (4 children with
one adult breaks the ratio), so she waits — fair enough. But now a 5th
child arrives. Five children with two adults is perfectly legal, and the
waiting adult *is still in the building*. Yet in the koan-33 style of
solution, the half-departed adult has already laid claim to part of the
center's capacity, and the child is turned away for no good reason.

Same rules as before, plus one:

1. **Invariant**: at every moment, `children_inside <= ratio * adults_inside`
   (an adult waiting to leave still counts as inside).
2. `child_enter` blocks only while admission would *actually* break the
   invariant; `adult_leave` blocks only while leaving would break it.
3. `adult_enter` and `child_leave` never block.
4. **No unnecessary waiting**: an adult stuck at the door must not stop a
   child from entering when the numbers genuinely allow it, and a waiting
   adult gets out the moment the numbers permit — not later.

## Your task

Edit `extended_child_care.py`. Implement `ExtendedChildCare(ratio=3)` with
the same four methods as koan 33:

- `adult_enter()` — never blocks.
- `adult_leave()` — blocks until this adult's departure is legal, and
  returns promptly once it is.
- `child_enter()` — blocks only while the ratio truly forbids entry.
- `child_leave()` — never blocks.

## Traps worth savoring

- Any design where a departing adult *reserves* capacity before it may
  leave fails the new requirement: the reservation is exactly the
  "unnecessary waiting" the tests hunt for. Koan 33's reference answer
  fails this koan.
- Waking a waiter and letting it re-check the world sounds easy, but a
  bare semaphore has no memory of *why* it was signaled. If your waiters
  decide for themselves after waking, ask what the world looks like by the
  time they run.

## Python notes

- The cleanest fix inverts responsibility: the thread that *changes* the
  counts decides who becomes unblocked, not the thread that waits. The
  book calls this "I'll do it for you"; it's the same shape as a
  `Condition` with the predicate evaluated by the notifier.
- `threading.Condition` would collapse this whole koan into two `wait_for`
  predicates — instructive to write once you've done it with semaphores,
  to see what the semaphore discipline was buying you.

Run: `./check python 34`
