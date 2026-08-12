# Koan 38 — Extended Faneuil Hall

*Adapted from* The Little Book of Semaphores, *§7.5 (CC BY-NC-SA 4.0).*

## The problem

Koan 37's solution has a loose end: an immigrant who already holds her
certificate can dawdle on the way out, and if the judge comes back for the
next ceremony while she is still inside, she is locked in for a whole
extra swearing-in. This koan closes that door.

All of koan 37's rules still apply:

1. While the judge is in the building, nobody else's `enter` may fire and
   no immigrant's `leave` may fire (spectators may leave freely).
2. `confirm` only after every entered immigrant has completed `check_in`.
3. `get_certificate` only after `confirm`.
4. Immigrants leave only after their certificate.

Plus the new rule:

5. After the judge's `leave` fires, **every immigrant sworn in at that
   ceremony must complete `leave` before the judge's next `enter` may
   fire** — even if the judge is already pacing outside the door.

## Your task

Edit `extended_faneuil_hall.hpp`. Same API as koan 37:
`ExtendedFaneuilHall` constructed with a `FaneuilHooks` struct
(`enter(who)`, `check_in(iid)`, `sit_down(iid)`, `swear(iid)`,
`get_certificate(iid)`, `confirm()`, `spectate(sid)`, `leave(who)`);
methods `immigrant(iid)`, `spectator(sid)`, `judge_visit()`.

`judge_visit()` may be called again while a previous visit is still
wrapping up — the new visit's `enter` simply may not fire until the
building has drained. A visit that finds no immigrants must still run to
completion.

## Traps worth savoring

- Reusing the koan 37 answer unchanged: the judge releases the building
  the moment she walks out, and visit #2 begins while ceremony #1's
  newly-sworn citizens are still gathering their coats. The tests stage
  exactly this ambush.
- Letting the judge simply reopen the front door for leavers: the door
  that lets immigrants *out* also lets the next crowd (and the judge)
  *in*. Exits need their own channel.
- Counting exits with a counter nobody locks — two immigrants leave at
  once, the count skips zero, and the judge waits forever.

## Modern C++ notes (many ways to skin this cat)

- Rule 5 is a **grace period**: phase N+1 may not begin until every
  participant of phase N has drained. This is literally the idea behind
  RCU's `synchronize_rcu()` and hazard-pointer reclamation — "nobody
  enters epoch e+1 until epoch e's readers are gone." You met it in the
  reusable-barrier koans; here it wears a judicial robe.
- The drain-to-zero shape is also what `std::latch` models
  (`count_down()` per leaver, one thread in `wait()`), and what a
  `condition_variable` predicate `remaining == 0` models. The book's
  baton dance shows it can be done with bare semaphores; in production
  you'd reach for the dedicated tool and spend your review budget
  elsewhere.
- Graceful shutdown of a thread pool or connection pool is this exact
  protocol: stop admitting, wait for in-flight work to drain, then flip
  the phase. Recognizing the shape means you already know where the bugs
  hide (the unlocked exit counter, the door that serves both directions).

Run: `./check cpp 38`
