# Koan 37 — Faneuil Hall

*Adapted from* The Little Book of Semaphores, *§7.5 (CC BY-NC-SA 4.0).*

## The problem

A naturalization ceremony at Boston's Faneuil Hall. Three kinds of threads
share the building: **immigrants**, who file in, check in, take a seat,
swear the oath, and collect their certificates; **spectators**, who wander
in to watch and wander out again; and one **judge**, who periodically
drops by to make it all official.

Each role calls a fixed sequence of hooks (see below). The hook call *is*
the action, so the rules are about when hooks are allowed to fire:

1. While the judge is in the building — from the moment her `enter` fires
   until her `leave` has fired — nobody else's `enter` may fire, and no
   immigrant's `leave` may fire. Spectators may leave whenever they like.
2. The judge's `confirm` may not fire until every immigrant whose `enter`
   has fired has also completed `check_in`.
3. No immigrant's `get_certificate` may fire before `confirm`.
4. An immigrant leaves only after receiving her certificate (which, with
   rule 1, means only after the judge has gone).

## Your task

Edit `faneuil_hall.py`. `FaneuilHall(hooks)` receives an object with eight
callables: `enter(who)`, `check_in(iid)`, `sit_down(iid)`, `swear(iid)`,
`get_certificate(iid)`, `confirm()`, `spectate(sid)`, `leave(who)`, where
`who` is `"immigrant:<id>"`, `"spectator:<id>"`, or `"judge"`. Implement:

- `immigrant(iid)` — enter → check_in → sit_down → swear →
  get_certificate → leave, obeying the rules.
- `spectator(sid)` — enter → spectate → leave.
- `judge_visit()` — enter → confirm → leave. One full visit per call; it
  may be called repeatedly for successive ceremonies, and a visit that
  finds no immigrants inside must still run to completion.

Hooks may block (the tests hold them open to freeze the ceremony
mid-scene); your synchronization must stay correct while they do.

## Traps worth savoring

- Tracking the judge with a bare flag that entrants read without any
  locking: an immigrant slips through the door in the same instant the
  judge walks in. The failure mode is an `enter` sandwiched between the
  judge's `enter` and `leave`.
- Having the judge wait for stragglers **while holding the lock that
  check-ins need**: the people she is waiting for are locked out of the
  very counter they must update. Total deadlock.
- Signaling "you may take your certificate" exactly once when several
  immigrants are seated: most of the room stays seated forever.

## Python notes

`threading.Semaphore.release(n)` (Python ≥ 3.9) can wake a whole batch
with one call — the semaphore equivalent of a broadcast, and a natural
fit for "everybody seated may now proceed" moments. Note the asymmetry
with `Condition.notify_all()`: banked semaphore tokens persist, so
late-arriving waiters still get through, while a notification that nobody
is waiting for evaporates.

Run: `./check python 37`
