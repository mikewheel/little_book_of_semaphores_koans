# Koan 23 — Building H₂O

*Adapted from* The Little Book of Semaphores, *§5.6 (CC BY-NC-SA 4.0).*

## The problem

Two species of thread — hydrogen and oxygen — arrive at an assembly point
and must leave as water molecules. Each molecule takes exactly two
hydrogens and one oxygen, and each atom calls `bond(...)` as it commits to
its molecule. The synchronization rules:

- A hydrogen that arrives without one more hydrogen *and* an oxygen ready
  must wait. An oxygen that arrives without two hydrogens ready must wait.
- Atoms never learn which specific threads they bonded with — the pairing
  is anonymous. The observable contract is on the *sequence* of `bond`
  calls: chop it into consecutive groups of three, and every group must
  contain exactly two `"H"` bonds and one `"O"` bond.
- No atom may return until all three atoms of its molecule have bonded.
  Leftover atoms wait for future arrivals; there is no partial credit.

## Your task

Edit `h2o.hpp`. `H2OBarrier` is constructed with an `H2OHooks` whose
`bond` callback you must invoke. Implement:

- `hydrogen()` — block until this thread can join a molecule as one of its
  two hydrogens, call `hooks_.bond("H")`, and return only after the whole
  molecule (both hydrogens and the oxygen) has bonded.
- `oxygen()` — the same, calling `hooks_.bond("O")`.

`bond` may be slow — the tests deliberately dawdle inside it. Molecules
must stay separated anyway.

Run: `./check cpp 23`

## Traps worth savoring

The tempting answer keeps two counters (or two counting semaphores),
releases a trio when the numbers line up, and calls it a day. Its failure
mode: the *next* trio can start bonding before the current trio finishes,
so with a slow `bond` the groups smear across molecule boundaries — some
window of three bonds is `H H H` or `H O O`. The tests carve the bond
sequence into triples and will show you exactly which molecule broke.

## Modern C++ notes (many ways to skin this cat)

- This puzzle escaped academia: it is LeetCode 1117 ("Building H2O") and a
  perennial systems-interview question. Worth having in your fingers.
- Two idiomatic C++ shapes exist. The semaphore shape batch-releases
  tokens: `counting_semaphore::release(2)` frees both hydrogens the moment
  a molecule is possible. The `condition_variable` shape parks everyone on
  one cv and wakes with a predicate like `h >= 2 && o >= 1`; it is easier
  to reason about but wakes threads spuriously, and you must re-check the
  predicate in a loop.
- Either way, the *grouping* guarantee needs more than counting: something
  must keep molecule `k+1` from bonding before molecule `k` is done. A
  small group barrier (`std::barrier<>` with 3 parties, or your koan-06
  build) is load-bearing here — delete it and the counts still balance
  while the triples interleave. The tests are written to notice.
- If a "mutex" can be released by a different thread than acquired it,
  `std::mutex` is the wrong tool (that's undefined behavior). A
  `std::binary_semaphore` has no owner and does this legally.
