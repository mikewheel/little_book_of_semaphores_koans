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

Edit `faneuil_hall.hpp`. `FaneuilHall` is constructed with a
`FaneuilHooks` struct of eight `std::function` callbacks: `enter(who)`,
`check_in(iid)`, `sit_down(iid)`, `swear(iid)`, `get_certificate(iid)`,
`confirm()`, `spectate(sid)`, `leave(who)`, where `who` is
`"immigrant:<id>"`, `"spectator:<id>"`, or `"judge"`. Implement:

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

## Modern C++ notes (many ways to skin this cat)

- This is your first *multi-role protocol object*: one class, three thread
  roles, each running a different script against shared state. In
  production such objects earn a class invariant comment and a lock-order
  note — the compiler checks neither, and three interleaved scripts is
  exactly where humans stop being able to simulate the state machine in
  their heads.
- `counting_semaphore::release(n)` is a *broadcast with memory*: `n`
  banked tokens that late arrivals can still collect. Contrast
  `condition_variable::notify_all()`, which only reaches threads already
  waiting — a thread that shows up a microsecond later missed it and must
  re-check its predicate. Semaphores carry state; notifications don't.
- Passing the ceremony as a struct of `std::function`s costs a heap
  allocation and an indirect call per hook — type erasure is not free.
  Here (a handful of calls per ceremony) that cost is irrelevant and the
  clarity wins. In a hot path you would template on a hooks type instead
  and let the calls inline.

Run: `./check cpp 37`
