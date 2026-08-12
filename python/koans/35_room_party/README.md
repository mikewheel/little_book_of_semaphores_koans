# Koan 35 — Room party

*Adapted from* The Little Book of Semaphores, *§7.3 (CC BY-NC-SA 4.0).*

## The problem

(The book's author wrote this one after a campus controversy about a dorm
room allegedly searched while its occupant was away.) A dorm room holds a
party; the Dean of Students prowls the hallway. Campus rules:

1. Any number of students may be in the room at once.
2. The dean may enter only under one of two conditions: the room is
   **empty** (to conduct a search), or the party has **more than
   `threshold` students** (to break it up).
3. If the dean shows up while the room holds between 1 and `threshold`
   students, the dean waits outside until one of those conditions comes
   true — however long that takes.
4. While the dean is in the room, **no student may enter**; students may
   still **leave**.
5. Once the dean starts breaking up a party, the dean stays in the room
   until every student has left. After a search (or once the room is
   cleared), the dean goes.
6. There is exactly one dean; you need not guard against a second one.

## Your task

Edit `room_party.py`. `Room(threshold=50, search=None, breakup=None,
party=None)` stores three test-supplied hooks for you (already wired in
the starter):

- `search()` — must be called exactly when the dean enters an empty room.
- `breakup()` — must be called exactly when the dean enters an
  over-threshold party.
- `party(sid)` — runs while student `sid` is in the room; it may take a
  while (the tests deliberately make students linger).

Implement:

- `student_visit(sid)` — enter the room (waiting outside while the dean is
  in it), call `party(sid)`, then leave. Returns once the student is out.
- `dean_visit()` — behave per rules 2–5; returns when the dean leaves.

Do not call `breakup()` or `search()` yourself from `student_visit`, and
call each at most once per `dean_visit`.

## Traps worth savoring

- The book flags this problem as *hard* and means it. The classic wrong
  dean either barges in on a legal small party (rule 3 gone), or checks
  the room once, sleeps, and re-checks — busy-waiting is not waiting.
- The subtle failure the book's first edition shipped: a dean who wakes
  up, walks in, and only *then* discovers that neither condition holds
  anymore — he can neither search nor break up and has to slink out. When
  the dean wakes, whoever woke him must have made one of the two
  conditions true, and it must *still* be true when he acts on it. Think
  hard about who hands the room's state to whom.
- Letting students trickle in one at a time past a dean who is mid-entry
  is a lost-wakeup factory.

## Python notes

- A `threading.Lock` may legally be released by a thread other than the
  one that acquired it — Python doesn't enforce lock ownership. That
  loophole is exactly what one classic solution style exploits (handing a
  held mutex from one thread to another); a `Semaphore(1)` works
  identically and advertises the intent better.
- If you'd rather not hand off locks, `threading.Condition` with
  `wait_for` re-checks predicates for you — but then the dean must
  re-derive *why* he was woken, which is this puzzle's whole difficulty in
  another costume.

Run: `./check python 35`
