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

Edit `baboons.py`. Implement `Rope(capacity=5)` with:

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
  rope forever; the westbound tests will starve — visibly.

## Python notes

If you solved koans 28–29 this is a reunion, not a new problem: swap the
labels and one constant. Noticing that two problems are isomorphic *is*
the skill — the bathroom, the rope, and half the "one shared conduit,
categorical users" systems you'll meet in production differ only in
costume.

Run: `./check python 30`
