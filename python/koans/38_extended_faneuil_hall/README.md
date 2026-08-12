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

Edit `extended_faneuil_hall.py`. Same API as koan 37:
`ExtendedFaneuilHall(hooks)` with hooks `enter(who)`, `check_in(iid)`,
`sit_down(iid)`, `swear(iid)`, `get_certificate(iid)`, `confirm()`,
`spectate(sid)`, `leave(who)`; methods `immigrant(iid)`, `spectator(sid)`,
and `judge_visit()`.

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

## Python notes

The new rule is a *drain* requirement: phase N+1 may not start until
phase N's participants are all gone. It is the same shape as a graceful
shutdown ("stop accepting requests, then wait for in-flight ones"), which
is why this variant is worth doing even after koan 37 clicks.

Run: `./check python 38`
