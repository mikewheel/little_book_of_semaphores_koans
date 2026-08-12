# Koan 15 — Dining philosophers

*Adapted from* The Little Book of Semaphores, *§4.4 (CC BY-NC-SA 4.0).*

## The problem

Dijkstra's 1965 classic. Five philosophers sit at a round table with one
plate each, a shared bowl of noodles, and — crucially — only five forks,
one between each pair of plates. A philosopher's life alternates between
thinking and eating, and eating requires *both* adjacent forks:

```
while (true) {
    think();
    get_forks(i);
    eat();
    put_forks(i);
}
```

Philosopher `i` sits between fork `i` (left) and fork `(i + 1) % 5`
(right), so each fork is contested by exactly two neighbors. The forks
stand for exclusive resources; the round table is what makes it
treacherous.

Your `get_forks` / `put_forks` must satisfy all four of:

1. A fork is held by at most one philosopher at a time — neighbors never
   eat simultaneously.
2. Deadlock is impossible, even when all five reach for forks at once.
3. No philosopher starves: whoever wants to eat, eventually eats (we
   assume `eat()` always finishes).
4. More than one philosopher can eat at the same time — a solution that
   feeds one philosopher at a time is safe but wasteful, and fails the
   tests.

## Your task

Edit `philosophers.hpp`. Implement `Table` (constructed with `n = 5`):

- `get_forks(i)` — block until philosopher `i` holds both adjacent forks.
- `put_forks(i)` — return both forks, waking anyone now able to eat.

The `left(i)` / `right(i)` index helpers are provided. The philosopher
loop itself lives in the tests; your class only manages forks.

## Traps worth savoring

- The symmetric grab — everyone picks up one fixed side first, then the
  other — passes a casual test run and then wedges solid: all five hold
  one fork and wait forever for a fork held by a neighbor doing exactly
  the same. A circular wait among identical actors is the textbook
  deadlock, and `no_deadlock_when_everyone_is_hungry` exists to force it
  out of hiding.
- Guarding the whole meal with one big lock kills the deadlock and the
  concurrency: `two_nonadjacent_philosophers_eat_together` will object.

## Modern C++ notes (many ways to skin this cat)

- If forks were `std::mutex`es, grabbing two of them naked —
  `fork_a.lock(); fork_b.lock();` — is *exactly* the book's non-solution,
  UB-free but deadlock-rich. The standard's answer is
  **`std::scoped_lock lk(fork_a, fork_b);`**: it locks any number of
  lockables with a deadlock-avoidance algorithm (`std::lock`'s
  try-and-back-off dance), which is the one-liner industrial fix for
  "I need these two resources atomically".
- The other production idiom is a **resource hierarchy**: give every lock
  a global rank and always acquire in ascending order. That's the
  "one leftie" solution wearing a suit — around the table, ordering fork
  indices makes exactly one philosopher reach for their left first.
- `std::counting_semaphore` forks can't ride `scoped_lock` (semaphores
  aren't Lockable — no ownership, any thread may release), which is why
  the koan makes you engineer the avoidance yourself instead of borrowing
  the library's.
- Starvation is the quiet second act: solutions that pass all tests here
  can still starve someone under an adversarial scheduler (see the hints
  about Tanenbaum's version). Fairness needs stronger tools than the
  tests can check — a reminder that "passes the suite" is evidence, not
  proof.

Run: `./check cpp 15`
