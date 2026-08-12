# The Little Book of Semaphores — Koans

Test-driven concurrency exercises in **Python** and **modern C++**, adapted
from the puzzles in Allen B. Downey's
[*The Little Book of Semaphores*](https://greenteapress.com/wp/semaphores/).

Every puzzle in the book is here as a *koan*: a small starter file with the
synchronization logic missing, plus a test suite that fails until you make it
correct. The tests check the real properties — ordering, mutual exclusion,
bounded concurrency, absence of deadlock and starvation — under adversarial
scheduling, so an "accidentally works" solution is unlikely to survive.

## Quick start

You need Docker (with Compose). Nothing else — no local Python or C++
toolchain required.

```sh
./check                # run every koan in both languages (first run builds images)
./check python 02      # run koan 02 in Python
./check cpp 02         # run koan 02 in C++
./check barrier        # run every koan matching "barrier", both languages
```

Or with raw Compose: `docker compose run --rm python koans/02_rendezvous` and
`docker compose run --rm cpp 02`.

## How to work a koan

1. `cd python/koans/02_rendezvous` (or `cpp/koans/02_rendezvous`).
2. Read `README.md` — the problem statement and the contract your code must
   satisfy. The C++ ones also discuss idiomatic choices ("many ways to skin a
   cat") where the community genuinely differs.
3. Edit the one starter file (`rendezvous.py` / `rendezvous.hpp`). Only that
   file — never the tests.
4. Run `./check python 02` until green. Repeat quickly; single koans run in
   seconds once images are built.
5. Stumped? Open `HINTS.md`. Hints are progressive: the first is a nudge, the
   last is nearly the book's approach. The problem statements and starter
   code never spoil anything.

Work top to bottom: later koans assume patterns you build in earlier ones
(turnstile, lightswitch, scoreboard, pass-the-baton, I'll-do-it-for-you).

## The koans

| #  | Koan | Book § | Challenge |
|----|------|--------|-----------|
| 01 | signaling | 3.1 | ● |
| 02 | rendezvous | 3.3 | ● |
| 03 | mutex | 3.4 | ● |
| 04 | multiplex | 3.5 | ● |
| 05 | barrier | 3.6 | ●● |
| 06 | reusable_barrier | 3.7 | ●●● |
| 07 | queue | 3.8 | ●● |
| 08 | exclusive_queue | 3.8 | ●●● |
| 09 | producer_consumer | 4.1 | ●● |
| 10 | bounded_buffer | 4.1 | ●● |
| 11 | readers_writers | 4.2 | ●● |
| 12 | no_starve_readers_writers | 4.2 | ●●● |
| 13 | writer_priority | 4.2 | ●●● |
| 14 | no_starve_mutex | 4.3 | ●●●● |
| 15 | dining_philosophers | 4.4 | ●●● |
| 16 | cigarette_smokers | 4.5 | ●●● |
| 17 | generalized_smokers | 4.5 | ●●● |
| 18 | dining_savages | 5.1 | ●● |
| 19 | barbershop | 5.2 | ●● |
| 20 | fifo_barbershop | 5.3 | ●●● |
| 21 | hilzers_barbershop | 5.4 | ●●●● |
| 22 | santa_claus | 5.5 | ●●● |
| 23 | h2o | 5.6 | ●●● |
| 24 | river_crossing | 5.7 | ●●● |
| 25 | roller_coaster | 5.8 | ●●● |
| 26 | multi_car_roller_coaster | 5.8 | ●●●● |
| 27 | search_insert_delete | 6.1 | ●●● |
| 28 | unisex_bathroom | 6.2 | ●● |
| 29 | no_starve_bathroom | 6.2 | ●●● |
| 30 | baboon_crossing | 6.3 | ●●● |
| 31 | modus_hall | 6.4 | ●●●● |
| 32 | sushi_bar | 7.1 | ●●● |
| 33 | child_care | 7.2 | ●● |
| 34 | extended_child_care | 7.2 | ●●● |
| 35 | room_party | 7.3 | ●●●● |
| 36 | senate_bus | 7.4 | ●●● |
| 37 | faneuil_hall | 7.5 | ●●● |
| 38 | extended_faneuil_hall | 7.5 | ●●●● |
| 39 | dining_hall | 7.6 | ●●● |
| 40 | extended_dining_hall | 7.6 | ●●●● |
| 41 | build_a_semaphore | 9.2 | ●●● |

(The handful of pencil-and-paper puzzles in chapters 1–2 — execution paths,
message-passing lunch protocols — don't translate to runnable koans; read
them in the book, they're short and fun.)

## What the tests can and cannot prove

Concurrency bugs are schedule-dependent. The suites stack the deck against
you — random jitter, stress iterations, forced-block probes, watchdog
timeouts that convert deadlocks into failures — but a passing run is strong
evidence, not a formal proof. The reverse direction is trustworthy: a
failing test is always a real violation of the contract.

If a test hangs and the watchdog fires, that *is* the failure: your solution
deadlocked. Read which test it was; the name tells you the scenario.

## Layout

```
check                  # one-command runner (wraps docker compose)
docker-compose.yml
python/
  Dockerfile           # python:3.12-slim + pytest
  koans/koan_utils.py  # shared test vocabulary (EventLog, OverlapTracker, …)
  koans/NN_name/       # README.md · HINTS.md · name.py · test_name.py
cpp/
  Dockerfile           # gcc:14 + cmake + ninja (C++20)
  common/koan_test.hpp # dependency-free test harness with deadlock watchdog
  koans/NN_name/       # README.md · HINTS.md · name.hpp · test_name.cpp
```

C++ builds are incremental across runs (cached in a named Docker volume).

## Attribution & license

The puzzles are the intellectual work of Allen B. Downey and the
contributors credited in *The Little Book of Semaphores* (2nd edition),
which is published under
[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).
Problem statements here are restated in this repository's own words; per the
ShareAlike term, this repository is likewise licensed under
**CC BY-NC-SA 4.0**. Read the book alongside these koans — the prose around
each puzzle (and each post-mortem of a broken "solution") is the best part.
