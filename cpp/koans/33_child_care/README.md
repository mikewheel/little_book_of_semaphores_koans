# Koan 33 — Child care

*Adapted from* The Little Book of Semaphores, *§7.2 (CC BY-NC-SA 4.0).*

## The problem

(The book credits this puzzle to Max Hailperin's *Operating Systems and
Middleware*.) A child care center runs under a licensing rule: each adult on
the floor may supervise at most three children. Adults and children come and
go all day, and the rule must hold at every instant — not just on average.

Formally, with `ratio = 3`:

1. **Invariant**: at every moment, `children_inside <= ratio * adults_inside`.
2. A child who shows up while the center is at capacity waits at the door
   until the bound has room again.
3. An adult may not walk out if that would leave too many children behind;
   `adult_leave` waits until enough children have gone home.
4. An arriving adult and a departing child never wait: `adult_enter` and
   `child_leave` must not block.

## Your task

Edit `child_care.hpp`. Implement `ChildCare` (constructed with `ratio`) with:

- `adult_enter()` — the adult is inside when it returns. Never blocks.
- `adult_leave()` — returns only when this adult may legally leave (the
  invariant still holds for the people remaining). May block.
- `child_enter()` — the child is inside when it returns. Blocks while
  admitting the child would break the invariant.
- `child_leave()` — the child is gone when it returns. Never blocks.

Any number of threads may call these concurrently.

## Traps worth savoring

There is a famous *almost*-solution here (the book walks right into it on
purpose). It handles every test you'd think to write with one adult — and
then two adults try to leave at the same time. Each of them gets partway
through claiming the "room" they need to take with them, neither can finish,
and the whole center freezes with the children still inside. If your
`two_adults_leaving` test times out with both leavers stuck, you have
reproduced a textbook deadlock: partial allocation, no preemption, circular
wait. The tests here provoke that schedule deliberately.

## Modern C++ notes (many ways to skin this cat)

- `std::counting_semaphore::release(n)` exists, but there is no
  `acquire(n)` — taking *n* permits atomically is your problem, and this
  koan is precisely about what goes wrong when you solve it carelessly.
  (`try_acquire` in a loop is not an answer either: backing out permits you
  already hold under contention is a livelock generator.)
- The condition-variable formulation dissolves the puzzle: keep `children`
  and `adults` under a `std::mutex` and have `child_enter` wait on
  `children < ratio * adults`, `adult_leave` on
  `children <= ratio * (adults - 1)`, with `notify_all` after every state
  change. No batch acquisition, no deadlock — the predicate is re-checked
  atomically on every wake. This is why cv-and-predicate is the default
  idiom in modern C++.
- If you go the semaphore route, you will end up *blocking on a semaphore
  while holding a mutex*. Usually that's a bug pattern (it stalls every
  other user of the mutex). Here it is provably safe — but only because of
  a structural fact worth stating in a comment: the sole threads that ever
  take that mutex are departing adults, and the permits a stalled leaver is
  waiting for are produced by `child_leave`/`adult_enter`, which never
  acquire it. Convince yourself there is no cycle; then write that argument
  down for the next reader.
- UB pitfall: releasing a `counting_semaphore` beyond its
  `least_max_value` ceiling is undefined behavior — miscount your permits
  and there's no `BoundedSemaphore` guard rail like Python's.

Run: `./check cpp 33`
