# Koan 30 — Baboon crossing

*Adapted from* The Little Book of Semaphores, *§6.3 (CC BY-NC-SA 4.0).*

## The problem

A single rope spans a canyon. Baboons cross hand-over-hand in either
direction — but two baboons headed opposite ways who meet mid-rope will
fight and fall, and more than 5 baboons at once will snap the rope.

Design the crossing protocol:

1. Once a baboon starts across, no baboon traveling the *opposite*
   direction may be on the rope until it (and everyone traveling with it)
   is off. Opposite directions never share the rope.
2. At most **5** baboons on the rope at any moment (default `capacity`).
3. Baboons going the *same* direction do share the rope — up to capacity
   at once. One-at-a-time is not an acceptable protocol.
4. No starvation: an unbroken stream of eastbound traffic must not keep a
   waiting westbound baboon (or vice versa) off the rope forever. Once a
   baboon is waiting, later arrivals from the opposing direction may not
   overtake it.

The book pointedly declines to print a solution to this one. The test
suite doesn't care: it checks the properties, not the pedigree.

## Your task

Edit `baboons.hpp`. Implement `Rope` (constructed with `capacity`,
default 5) with:

- `east_enter()` / `east_exit()` — bracket an eastbound crossing;
  `east_enter` blocks until getting on the rope is legal.
- `west_enter()` / `west_exit()` — same for westbound.

Enter/exit calls are always correctly paired by the callers.

## Traps worth savoring

- A capacity cap with no notion of direction: the rope never snaps, but
  east and west meet in the middle. The mixing test will name the corpse.
- Direction exclusion with no cap: one direction is safe by itself, so 8
  eager eastbound baboons pile on. Snap.
- Both of the above but no fairness: heavy eastbound traffic holds the
  rope forever; the westbound test will starve — visibly.

## Modern C++ notes (many ways to skin this cat)

- This shape — directional batches taking turns over one shared conduit —
  is everywhere in systems work: half-duplex bus arbitration, single-track
  railway signaling, TCP-connection direction draining, readers/writers
  with two writer classes. The costume changes; the semaphore roster
  doesn't.
- The real-world tuning knob is the **batching window**: how long one
  direction keeps the conduit before yielding. A bare turnstile yields as
  soon as anyone opposes, which is maximally fair and minimally
  throughput-friendly (small convoys, frequent turnarounds — every
  turnaround drains the pipeline). Industrial designs add hysteresis: let
  up to N cross, or hold for T microseconds, before flipping. Know which
  knob your workload wants before hand-rolling it.
- One `std::counting_semaphore<>` can serve as the weight limit for both
  directions, because exclusivity means only one direction draws tokens
  at a time — but only if exiting baboons return the token *before* the
  rope changes hands. Order of releases matters; convince yourself, then
  write the comment your reviewer will need.
- `std::binary_semaphore` for the turnstile documents its 0/1 protocol in
  the type. Signaling it above 1 is UB, not a bug you get to observe.

Run: `./check cpp 30`
