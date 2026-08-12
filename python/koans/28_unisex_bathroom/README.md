# Koan 28 — Unisex bathroom

*Adapted from* The Little Book of Semaphores, *§6.2 (CC BY-NC-SA 4.0).*

## The problem

An office building has one convenient bathroom, and management agrees to
make it unisex — under strict conditions:

- Men and women are never inside at the same time.
- At most `capacity` (classically 3) people are inside at once.
- Up to `capacity` people of the *same* gender may share — a solution
  that admits one person at a time is over-locked and unacceptable.
- No deadlock.

Starvation, for now, is allowed: an unbroken parade of women may keep men
waiting indefinitely (koan 29 repairs that injustice). One more subtlety
worth reading twice: when the bathroom holds two women and a man is
waiting, a woman leaving does **not** let him in — he enters only when
the room is completely *empty*.

## Your task

Edit `bathroom.py`. Implement `Bathroom(capacity=3)` with:

- `female_enter()` / `female_exit()` — entry blocks while any man is
  inside or while `capacity` women already are.
- `male_enter()` / `male_exit()` — symmetric.

## Traps worth savoring

- A plain multiplex of size 3 with no notion of gender: capacity is
  respected, and the genders happily mingle. The tests snapshot occupancy
  at every entry and will list the mixed company.
- A single "the room is mine" semaphore per entrant: genders never mix,
  but the second woman blocks behind the first. The sharing test catches
  this over-serialization.
- Letting a waiting man in the moment a slot frees rather than when the
  room empties — the trap called out above; there is a dedicated test.

Run: `./check python 28`

## Python notes

If you extracted the counter-plus-first-in-locks helper from koan 27,
this koan is two instances of it plus two multiplexes. If you didn't,
now is the moment — koan 29 will want it a third time.
