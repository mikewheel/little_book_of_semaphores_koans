# Koan 39 — Dining Hall

*Adapted from* The Little Book of Semaphores, *§7.6 (CC BY-NC-SA 4.0).*

## The problem

Students drift through the dining hall: each one dines, then leaves.
Between finishing her food and actually walking out she is *ready to
leave*. House etiquette says nobody may be abandoned mid-meal: a student
must not walk out if that would leave **exactly one** other student still
eating with no other ready-to-leave student keeping her company.

Work out when a departing student actually has to wait and you'll find
there is exactly one sticky situation — and two different events that can
un-stick it:

- a newcomer sits down to eat (two diners now — company restored), or
- the lone diner finishes too (nobody is eating anymore, and the two of
  them walk out together).

Everything else — leaving a table of many, leaving an empty table —
is unrestricted, and dining itself never requires waiting in this koan.

## Your task

Edit `dining_hall.hpp`. `DiningHall` is constructed with a `DiningHooks`
struct holding two callbacks: `dine(sid)` and `leave(sid)`. Implement:

- `student(sid, dine_gate)` — one student's whole visit. She calls
  `hooks_.dine(sid)` when she starts eating; if `dine_gate` is non-empty,
  she calls it after the dine hook returns (the tests pass a blocking
  callable to keep her at the table). Once done eating she is ready to
  leave, and `hooks_.leave(sid)` must fire only when etiquette allows.

The hooks may block — the tests hold `dine` open to control exactly who
is eating when — and your synchronization must stay correct while they do.

## Traps worth savoring

- No synchronization at all *almost* works — most of the time nobody is
  stranded. The tests stage the one situation where etiquette bites: an
  early finisher walking out on a lone diner.
- Making the *blocked* leaver reacquire the shared state after she is
  woken invites a lost-update race with the very student who woke her.
  There is a classic pattern where the waker updates the counters on the
  sleeper's behalf; the book names it "I'll do it for you".
- Blocking a leaver whenever anyone is still eating over-constrains: a
  table of five must let three walk out freely. The tests check progress
  as well as politeness.

## Modern C++ notes (many ways to skin this cat)

- This is a *tiny state machine*: two counters, a handful of reachable
  states, one blocking transition. At this size, an exhaustive case
  analysis under a single `std::mutex` beats any amount of semaphore
  cleverness — you can enumerate every branch and convince a reviewer in
  one sitting. Save the lock-free heroics for state spaces that earn them.
- The same logic writes naturally as a `condition_variable` with predicate
  "leaving now strands nobody": `cv.wait(lock, pred)` re-checks after
  every wakeup, which quietly replaces the "I'll do it for you" bookkeeping
  hand-off the semaphore version needs. Compare both shapes; knowing when
  a CV predicate is simpler than a baton is a real design skill.
- Etiquette rules like this are *testable* precisely because the protocol
  object takes its observable actions as injected callables — the tests
  freeze a diner mid-bite and stage the exact stranding scenario. The same
  dependency-injection trick makes production schedulers and pools
  testable; an object that calls opaque internals cannot be staged.

Run: `./check cpp 39`
