# Koan 31 — The Modus Hall problem

*Adapted from* The Little Book of Semaphores, *§6.4 (CC BY-NC-SA 4.0).*

## The problem

After a snowstorm, a single-file trench connects the Mods shantytown to
the rest of campus. Two factions use it: Mods "heathens" and residence
hall "prudes". Members of the *same* faction squeeze past each other
happily, so a faction can share the trench without limit. When opposing
factions meet, nobody negotiates — **the larger group wins**, and the
smaller one waits in the snow.

This is categorical exclusion (koans 28–30) with a twist: which category
holds the path is decided by *majority rule*, which also makes it
starvation-resistant — the losing side accumulates waiters until it
outnumbers the incumbents, then takes the path as a batch.

The precise rules your `Path` must enforce:

1. Both factions are never on the path at the same time.
2. Any number of one faction may be on the path together.
3. When the path is empty, the first arrival's faction takes it.
4. While faction X holds the path, arriving Y members queue. The moment
   the *queued* Y members outnumber the X members currently present, the
   balance tips: X arrivals from then on must queue (no new X may start
   crossing), the X members already on the path finish, and then the
   whole waiting Y cohort crosses.
5. If Y's waiters never achieve a majority, they keep waiting, and X
   members keep passing freely — that's the (fair-ish) price of rule 4.

Counting note, straight from the book: "majority" is measured over
threads that have *checked in* — a thread that has arrived but not yet
registered itself isn't a voter yet.

## Your task

Edit `modus_hall.hpp`. Implement `Path` with:

- `heathen_cross(cross)` — arrive as a heathen, wait if the rules demand
  it, then run `cross()` while on the path, then leave.
- `prude_cross(cross)` — the same for prudes.

Both take a `const std::function<void()>&` and must be callable from
many threads at once.

## Traps worth savoring

- A single shared turnstile (the koan-29/30 recipe) enforces plain
  first-come categorical exclusion — *minority rule included*: two lone
  prudes at the turnstile can halt an army of heathens. Rule 5 dies, and
  the test for it will tell you so.
- Forgetting that the tipping arrival must *lock out* its opponents at
  the door as well as queue itself: without that, X keeps streaming in
  and the Y cohort never gets its turn.
- Releasing the waiting cohort one-by-one instead of as a batch invites
  interleavings where a fresh opponent slips into the gap.

## Modern C++ notes (many ways to skin this cat)

- Two schools solve problems like this. **Semaphore choreography** passes
  permission around as tokens; it shines when the protocol is a pipeline.
  **A state machine under a mutex** (a scoreboard) shines when the rules
  are a case analysis — like majority rule. Here the scoreboard wins:
  every arrival/departure takes one mutex, consults the counters and a
  status, and parks on a plain semaphore if told to. The semaphores hold
  no cleverness at all.
- Model the status as an `enum class Status { Neutral, HeathensRule,
  PrudesRule, TransitionToHeathens, TransitionToPrudes };` and dispatch
  with a `switch`. With `-Wall`, a `switch` over an `enum class` with no
  `default:` warns on unhandled enumerators — exhaustiveness checking for
  free. (Once you add `default:` you've silenced your safety net; prefer
  naming every case.)
- `std::counting_semaphore<>::release(n)` releases the whole cohort in
  one call; batching wakeups also batches the context switches.
- The blocking `acquire()` of a turnstile inside the mutex-held region is
  intentional and safe *here* (the turnstile is only ever held
  transiently by a passer-through) — but every such "lock held while
  blocking" line deserves a written proof in review. This one's in the
  hints.

Run: `./check cpp 31`
