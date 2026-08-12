# Koan 36 — Senate bus

*Adapted from* The Little Book of Semaphores, *§7.4 (CC BY-NC-SA 4.0).*

## The problem

(Modeled on the campus shuttle at Wellesley College.) Riders drift up to a
bus stop and wait. Eventually a bus pulls in, and the rules of the route
are strict:

1. When the bus arrives, exactly the riders **already waiting at that
   moment** board — up to the bus's `capacity`. If more are waiting, the
   excess stays behind for the next bus.
2. Anyone who walks up **while boarding is in progress** is too late: they
   wait for the next bus, no matter how much space this one has.
3. Once every boarding rider is aboard, the bus calls `depart(n)` with the
   number who boarded, and drives off.
4. A bus that pulls up to an empty stop departs immediately: `depart(0)`.

## Your task

Edit `senate_bus.hpp`. `BusStop(capacity, board, depart)` stores two
test-supplied hooks for you (already wired in the starter):

- `board(rid)` — must run once for rider `rid` as that rider boards.
- `depart(n)` — must run once per bus visit, with the exact number who
  boarded this bus.

Implement:

- `rider(int rid)` — arrive at the stop, wait for a bus, board it (the
  call returns once rider `rid` is aboard).
- `bus_arrives()` — one bus visit: board the eligible waiting riders (up
  to `capacity`, none that arrived after the bus), call `depart(n)`, and
  return.

Buses arrive one at a time (no two buses at the stop at once), but riders
arrive whenever they please — including mid-boarding.

## Traps worth savoring

- The tempting shortcut is to treat the stop as a simple multiplex: bus
  arrives, hands out permits, riders take them as they come. Everything
  passes — until a rider strolls up *during* boarding and grabs a seat
  they were never entitled to. Rule 2 is a **cutoff**, and a cutoff needs
  a moment-in-time snapshot, not a running count.
- Counting departures wrong is the other classic: `depart(n)` must report
  who *this* bus took, not how many were ever waiting.

## Modern C++ notes (many ways to skin this cat)

- The winning shape is **snapshot-then-serve**: under the lock, capture
  `n = min(waiting, capacity)` and serve exactly `n`. It is the same
  publish-a-consistent-view idiom as a GC safepoint or an RCU grace
  period: freeze membership first, act on the frozen set, let newcomers
  queue for the next epoch. Once you see the koan this way, the mutex's
  *scope* (held across all of boarding) stops looking heavy-handed and
  starts looking like the specification.
- `depart(n)` is a completion callback. Note the discipline the tests
  force: it fires after the last `board()` of this batch and before the
  method returns — exactly the contract you'd document for any async
  batch API.
- A Go programmer would write this with channels: riders send themselves
  into a `chan`, the bus drains up to `capacity` items *currently
  buffered* and refuses to block on stragglers. C++ has no buffered
  channel in the standard library; the `waiting` counter plus two
  semaphores is that channel, hand-rolled — which is precisely why the
  cutoff doesn't come for free here.
- If you batch-release with `release(n)`, ask what stops the *next*
  batch's early birds from consuming this batch's permits. The
  per-rider handshake in the classic solution is not politeness; it's
  flow control.

Run: `./check cpp 36`
