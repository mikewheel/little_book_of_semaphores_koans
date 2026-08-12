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

Edit `fair_bathroom.hpp`. Implement `FairBathroom` (constructed with
`capacity`, default 3) with:

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

## Modern C++ notes (many ways to skin this cat)

- Reuse the `Lightswitch` you wrote for koan 28 — this koan is that roster
  plus exactly one more semaphore. Composition over cleverness.
- Fairness costs throughput, here and everywhere. Batching whole genders
  (koan 28) keeps the room full; the fair version tends to admit small
  alternating convoys, so under contention the room often holds one or two
  people instead of three. The same trade-off shows up in `std::mutex`
  (unfair, fast — a releasing thread can immediately re-acquire) versus
  ticket locks and fair queued locks (no barging, more context switches).
  Don't buy fairness until you've measured the starvation you're curing.
- Nothing in `std::` hands you the waiting-room behavior directly; a
  `std::condition_variable` solution needs explicit generation/queue
  bookkeeping to stop late arrivals from barging, because `notify_all`
  wakes waiters into an ordinary (unfair) mutex race.
- The enter/exit pairs scream RAII: in production, return a move-only
  guard object from `enter()` whose destructor exits, so an exception
  can't leave a phantom occupant. The koans keep the raw calls because
  later puzzles build on this vocabulary.

Run: `./check cpp 29`
