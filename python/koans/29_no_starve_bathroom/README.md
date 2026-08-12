# Koan 29 — No-starve unisex bathroom

*Adapted from* The Little Book of Semaphores, *§6.2 (CC BY-NC-SA 4.0).*

## The problem

Koan 28's bathroom keeps the genders apart and caps the headcount, but it
has an ugly failure mode: as long as women keep trickling in, an arriving
man can wait *forever* (and vice versa). Each newcomer of the incumbent
gender extends the occupation indefinitely. Your job is the same bathroom
with the starvation hole plugged.

The constraints:

1. Men and women are never inside at the same time.
2. At most `capacity` (default 3) people are inside at once.
3. People of the same gender may share the room, up to `capacity`.
4. **No starvation**: once someone is waiting, people of the opposite
   gender who arrive *after* them must not get in ahead of them. The
   current occupants may finish up, but the door then goes to the waiter
   before any late arrivals.

## Your task

Edit `fair_bathroom.py`. Implement `FairBathroom(capacity=3)` with:

- `male_enter()` / `male_exit()` — bracket a man's visit. `male_enter`
  blocks until he may go in without violating the constraints above.
- `female_enter()` / `female_exit()` — same for women.

Enter/exit calls are always correctly paired by the callers; you don't
need to defend against a stray `exit`.

## Traps worth savoring

- A solution that is *safe* but not *fair* — koan 28's exact code — passes
  every test except the fairness ones: a late arrival of the incumbent
  gender waltzes past the queue. If those two tests are the only red ones,
  you've solved the wrong koan.
- Blocking *both* genders too aggressively is the opposite failure: if
  same-gender friends can no longer share the room, you've serialized the
  bathroom into a mutex.

## Python notes

The fair version deliberately trades throughput for fairness: under mixed
load it tends to admit small alternating convoys instead of long
same-gender streams, so the room is rarely full. That trade-off is real in
production too (fair locks are slower than unfair ones — see
`threading.Lock` vs. handoff-style fair queues). Measure before you pay
for fairness you don't need.

Run: `./check python 29`
