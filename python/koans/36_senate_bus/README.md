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

Edit `senate_bus.py`. `BusStop(capacity=50, board=None, depart=None)`
stores two test-supplied hooks for you (already wired in the starter):

- `board(rid)` — must run once for rider `rid` as that rider boards.
- `depart(n)` — must run once per bus visit, with the exact number who
  boarded this bus.

Implement:

- `rider(rid)` — arrive at the stop, wait for a bus, board it (the call
  returns once rider `rid` is aboard).
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

## Python notes

- The snapshot-then-serve shape here (freeze the eligible set, serve
  exactly that set, let newcomers accumulate for the next round) shows up
  all over real systems: GC safepoints, batch queue drains, double
  buffering.
- `threading.Semaphore.release(n)` can wake a whole batch at once, but
  think through what stops batch n+1's early birds from stealing batch
  n's wakeups before you reach for it.

Run: `./check python 36`
