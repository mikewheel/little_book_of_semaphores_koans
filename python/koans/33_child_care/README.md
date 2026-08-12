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
   until another adult arrives (or a child leaves is no help — the bound is
   already tight — so really: until the bound has room).
3. An adult may not walk out if that would leave too many children behind;
   `adult_leave` waits until enough children have gone home.
4. An arriving adult and a departing child never wait: `adult_enter` and
   `child_leave` must not block.

## Your task

Edit `child_care.py`. Implement `ChildCare(ratio=3)` with:

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

## Python notes

- `threading.Semaphore.release(n)` can hand out several permits at once, but
  `acquire()` has no counterpart that takes several — taking *n* permits is
  necessarily a loop, and how you protect that loop is the heart of this koan.
- `threading.BoundedSemaphore` will throw if your bookkeeping ever releases
  more than it should — cheap insurance while you iterate.

Run: `./check python 33`
