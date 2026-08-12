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

Edit `extended_dining_hall.py`. `ExtendedDiningHall(hooks)` receives an
object with three callables: `get_food(sid)`, `dine(sid)`, `leave(sid)`.
Implement:

- `student(sid, dine_gate=None)` — one student's whole visit. She calls
  `hooks.get_food(sid)` first; `hooks.dine(sid)` fires only when the
  sitting rule allows; if `dine_gate` is given she calls it after the
  dine hook returns (the tests pass a blocking callable to keep her at
  the table); `hooks.leave(sid)` fires only when the leaving rule allows.

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

## Python notes

Three counters, one lock, two park benches (semaphores) — the scoreboard
scales linearly while the case analysis grows combinatorially. Writing
the checkout and check-in as explicit, exhaustive if/elif chains is not
inelegant; at this size it is the *legible* design.

Run: `./check python 40`
