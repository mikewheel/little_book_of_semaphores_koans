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

Edit `coaster.py`. `RollerCoaster(capacity, hooks)` stores both. Implement:

- `passenger(pid)` — one complete ride: wait for permission to board, call
  `self.hooks.board(pid)`, ride, wait for permission to unboard, call
  `self.hooks.unboard(pid)`, return.
- `start_car(n_rides)` — the car's body (the tests give it its own
  thread): `n_rides` cycles of load / full-car run / unload, returning
  when the last cycle's riders are all ashore.

## Traps worth savoring

- The car calls `run()` right after handing out `capacity` boarding
  passes — without waiting for the passes to be *used*. The tests hold a
  seat hostage and watch whether the car leaves anyway.
- Some poor thread ends up responsible for a counter it doesn't own:
  decide precisely who resets the boarded-count and when, or cycle two
  inherits cycle one's arithmetic.
- Signaling "all aboard" once per passenger instead of once per full car —
  the car takes off after the first click of the seatbelt.

Run: `./check python 25`

## Python notes

The two directions of this problem are asymmetric: the car fans *out*
permissions (`Semaphore.release(n)` hands out `n` boarding passes at
once), while passengers fan *in* a completion signal (the last one in
signals once). Getting fan-out and fan-in confused is the classic way to
lose a passenger.
