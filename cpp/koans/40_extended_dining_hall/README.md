# Koan 40 — Extended Dining Hall

*Adapted from* The Little Book of Semaphores, *§7.6 (CC BY-NC-SA 4.0).*

## The problem

Same dining hall, one more course. Students now `get_food`, then `dine`,
then `leave`. Between getting food and dining a student is *ready to eat*;
between dining and leaving she is *ready to leave*.

The etiquette rule doubles up — a student may never sit at the table
alone, on either end of the meal:

- She may not **start** dining while nobody else is at the table and no
  other student is ready to eat. She waits with her tray until a second
  ready-to-eat student shows up (the two sit down together) — unless
  someone is already dining, in which case she joins immediately.
- She may not **leave** if that would strand exactly one diner with no
  other ready-to-leave student for company (koan 39's rule, unchanged):
  the stranded case resolves when a newcomer starts dining or the lone
  diner finishes and the two leave together.

The pleasant surprise of the analysis: each end has exactly one blocking
situation, and they never interact — you cannot strand a leaver at an
empty table, and you cannot need company to sit when someone's eating.

## Your task

Edit `extended_dining_hall.hpp`. `ExtendedDiningHall` is constructed with
a `DiningHooks` struct holding three callbacks: `get_food(sid)`,
`dine(sid)`, `leave(sid)`. Implement:

- `student(sid, dine_gate)` — one student's whole visit. She calls
  `hooks_.get_food(sid)` first; `hooks_.dine(sid)` fires only when the
  sitting rule allows; if `dine_gate` is non-empty she calls it after the
  dine hook returns (the tests pass a blocking callable to keep her at
  the table); `hooks_.leave(sid)` fires only when the leaving rule allows.

The hooks may block; your synchronization must stay correct while they do.

## Traps worth savoring

- Porting koan 39 and forgetting the tray line: your first student sits
  down all alone at an empty table. The very first test stages exactly
  her.
- Waking the waiting would-be diner without seating her: the waker must
  fix the ready-to-eat/eating counts for **both** of them, or the counts
  drift and someone later waits forever.
- When a pair sits down together at an empty table there is nobody to
  strand, so checking the leave-side rescue in that branch is dead code —
  but *forgetting* the rescue in the join-an-occupied-table branch leaves
  koan 39's waiter stuck.

## Modern C++ notes (many ways to skin this cat)

- Koan 39's case analysis doubled when the protocol grew one phase. This
  is the standard trajectory: hand-written if/else state machines scale
  combinatorially, which is why bigger protocols move to *table-driven*
  state machines — an enum of states, a transition table, one place where
  transitions are legal. When your branches stop fitting on one screen,
  that's the signal.
- With `condition_variable` you could collapse both park benches into two
  predicates: `cv.wait(lock, [&]{ return eating_ > 0 || ready_to_eat_ >= 2; })`
  on the way in, and the koan 39 predicate on the way out. Predicate
  waits re-check state on every wakeup, so the "waker does the sleeper's
  bookkeeping" choreography disappears — a genuine simplification worth
  weighing against the semaphore version you just wrote.
- Etiquette invariants like "never exactly one diner while leavers idle"
  are ideal *property tests*: run randomized traffic, sample the state at
  every transition, assert the invariant. The scenario tests here pin the
  known-tricky interleavings; a property harness would hunt the unknown
  ones. Both belong in a serious concurrency test kit.

Run: `./check cpp 40`
