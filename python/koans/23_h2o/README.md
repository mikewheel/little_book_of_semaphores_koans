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

Edit `h2o.py`. `H2OBarrier(hooks)` receives a hooks object exposing
`bond(kind)`. Implement:

- `hydrogen()` — block until this thread can join a molecule as one of its
  two hydrogens, call `self.hooks.bond("H")`, and return only after the
  whole molecule (both hydrogens and the oxygen) has bonded.
- `oxygen()` — the same, calling `self.hooks.bond("O")`.

`bond` may be slow — the tests deliberately dawdle inside it. Molecules
must stay separated anyway.

## Traps worth savoring

The tempting answer keeps two counters (or two counting semaphores),
releases a trio when the numbers line up, and calls it a day. Its failure
mode: the *next* trio can start bonding before the current trio finishes,
so with a slow `bond` the groups smear across molecule boundaries — some
window of three bonds is `H H H` or `H O O`. The tests carve the bond
sequence into triples and will show you exactly which molecule broke.

Run: `./check python 23`

## Python notes

`threading.Semaphore.release(n)` can hand out several tokens in one call —
handy when a whole molecule's worth of threads becomes eligible at once.
And note that Python's GIL does not save you here: `bond` sleeps, and
sleeping threads interleave freely.
