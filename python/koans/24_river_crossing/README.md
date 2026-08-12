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

Edit `river.py`. `Boat(hooks)` receives a hooks object with `board(kind)`
and `row_boat(kind)`. Implement:

- `hacker_arrives()` — block until this thread is part of a legal crew of
  four, call `self.hooks.board("hacker")`, and return once the boatload
  has sailed. If this thread is the one rowing, call
  `self.hooks.row_boat("hacker")` after all four boards.
- `serf_arrives()` — the same with kind `"serf"`.

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

Run: `./check python 24`

## Python notes

Each thread needs a private "am I the rower?" fact. Resist the urge to
store it on `self` — anything on the shared object is, by definition, not
private to a thread. A humble local variable is the tool.
