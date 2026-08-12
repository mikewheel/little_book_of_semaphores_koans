# Koan 25 — Roller coaster

*Adapted from* The Little Book of Semaphores, *§5.8 (CC BY-NC-SA 4.0).*

## The problem

An amusement park has one roller-coaster car with `capacity` seats and a
crowd of passenger threads (more passengers than seats). The car cycles;
the passengers ride. Hooks mark every phase: the car calls `load()`,
`run()`, `unload()`; each passenger calls `board(pid)` and `unboard(pid)`.
The choreography:

- Passengers may board only after the car has called `load()` — and only
  `capacity` of them per cycle. Everyone else waits for a later cycle.
- The car may call `run()` only after all `capacity` passengers have
  boarded. A half-empty coaster never leaves the station.
- Passengers may unboard only after the car has called `unload()`.
- The car may start the next `load()` only after all `capacity` riders of
  the previous cycle have unboarded.

## Your task

Edit `coaster.hpp`. `RollerCoaster(capacity, hooks)` stores both.
Implement:

- `passenger(pid)` — one complete ride: wait for permission to board, call
  `hooks_.board(pid)`, ride, wait for permission to unboard, call
  `hooks_.unboard(pid)`, return.
- `start_car(n_rides)` — the car's body (the tests give it its own
  thread): `n_rides` cycles of load / full-car run / unload, returning
  when the last cycle's riders are all ashore.

Run: `./check cpp 25`

## Traps worth savoring

- The car calls `run()` right after handing out `capacity` boarding
  passes — without waiting for the passes to be *used*. The tests hold a
  seat hostage and watch whether the car leaves anyway.
- Some poor thread ends up responsible for a counter it doesn't own:
  decide precisely who resets the boarded-count and when, or cycle two
  inherits cycle one's arithmetic.
- Signaling "all aboard" once per passenger instead of once per full car —
  the car takes off after the first click of the seatbelt.

## Modern C++ notes (many ways to skin this cat)

- The car-to-passenger direction is a **batch handoff**:
  `counting_semaphore::release(capacity)` fans out a cycle's worth of
  permissions in one call. The passenger-to-car direction is fan-in — a
  mutex-guarded counter whose last incrementer signals once. Together they
  generalize the barrier you built in koan 06: a barrier is just fan-in
  followed by fan-out.
- C++20's `std::latch` is a tidy alternative for each phase: create a
  `latch(capacity)`, passengers `count_down()`, the car `wait()`s. But a
  latch is single-use, so you would need a fresh pair per cycle — that
  allocation-per-cycle is the price of not reusing semaphores.
- Whoever resets the boarded-counter owns a subtle invariant: the reset
  must happen before the same counter is used by the next cycle. Here the
  last boarder resets while still holding the counter's mutex — the reset
  and the "full car" signal are atomic together. Separating them reopens
  the race.
- The counters are plain `int`s guarded by `std::mutex` — this is the rare
  koan where `std::mutex` + `std::lock_guard` is exactly right (same
  thread locks and unlocks), unlike koans 23/24's ownerless handoffs.
