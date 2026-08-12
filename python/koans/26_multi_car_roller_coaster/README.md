# Koan 26 — Multi-car roller coaster

*Adapted from* The Little Book of Semaphores, *§5.8 (multi-car variant, CC BY-NC-SA 4.0).*

## The problem

The park got popular: the single track now carries `n_cars` cars, each
with `capacity` seats. Everything from koan 25 still holds per car, plus
the physics of sharing one track:

- Only one car may be boarding passengers at any moment — there is one
  loading dock.
- Cars take the dock in fixed rotation: car 0 loads first, then 1, then 2,
  … wrapping back to 0 for each car's next ride.
- Multiple cars may be out on the track simultaneously — that's the point
  of owning several cars.
- Cars cannot overtake each other on the track, so they must unload in
  exactly the order they loaded.
- All riders of one carload unboard before any rider of the next carload.

Hooks now carry the car id: the car calls `load(car)`, `run(car)`,
`unload(car)`; passengers still call `board(pid)` / `unboard(pid)`.

## Your task

Edit `multicar_coaster.py`. `MultiCarCoaster(n_cars, capacity, hooks)`
stores all three. Implement:

- `passenger(pid)` — one complete ride, same contract as koan 25: wait,
  `board(pid)`, ride, wait, `unboard(pid)`, return.
- `start_cars(n_rides_per_car)` — launch one thread per car (car `i`
  knows its own id), each doing `n_rides_per_car` cycles under the rules
  above; return when every car has finished.

The tests always supply exactly `n_cars * capacity * n_rides_per_car`
passengers, so every seat of every ride is eventually filled.

## Traps worth savoring

- A koan-25 car cloned three times with no further coordination: two cars
  call `load` at once and passengers from different carloads interleave.
  The tests read the event log as nested windows and name the guilty cars.
- Coordinating loading but not unloading: cars come home in scrambled
  order. Watch the `unload` sequence.
- Off-by-one in the rotation: car 0 must go first, and the car after
  `n_cars - 1` is car 0 again.

Run: `./check python 26`

## Python notes

A list comprehension builds a ring of semaphores neatly — and note that
the ring's *initial state* encodes "whose turn is first". Only one member
of each ring starts unlocked.
