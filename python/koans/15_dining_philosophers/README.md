# Koan 15 — Dining philosophers

*Adapted from* The Little Book of Semaphores, *§4.4 (CC BY-NC-SA 4.0).*

## The problem

Dijkstra's 1965 classic. Five philosophers sit at a round table with one
plate each, a shared bowl of noodles, and — crucially — only five forks,
one between each pair of plates. A philosopher's life alternates between
thinking and eating, and eating requires *both* adjacent forks:

```
while True:
    think()
    get_forks(i)
    eat()
    put_forks(i)
```

Philosopher `i` sits between fork `i` (left) and fork `(i + 1) % 5`
(right), so each fork is contested by exactly two neighbors. The forks
stand for exclusive resources; the round table is what makes it treacherous.

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

Edit `philosophers.py`. Implement `Table(n=5)`:

- `get_forks(i)` — block until philosopher `i` holds both adjacent forks.
- `put_forks(i)` — return both forks, waking anyone now able to eat.

The `left(i)` / `right(i)` index helpers are provided. The philosopher
loop itself lives in the tests; your class only manages forks.

## Traps worth savoring

- The symmetric grab — everyone picks up one fixed side first, then the
  other — passes a casual test run and then wedges solid: all five hold
  one fork and wait forever for a fork held by a neighbor doing exactly
  the same. A circular wait among identical actors is the textbook
  deadlock, and `test_no_deadlock_when_everyone_is_hungry` exists to
  force it out of hiding.
- Guarding the whole meal with one big lock kills the deadlock and the
  concurrency: `test_two_nonadjacent_philosophers_eat_together` will
  object.

## Python notes

The fix-one-condition escape hatches here (limit the diners, break the
symmetry, or track states under a mutex) are general deadlock medicine,
not philosopher trivia — the same shapes appear in database lock managers
and connection pools. Python's `with a_lock, b_lock:` statement acquires
in source order and so is just the symmetric grab in nicer clothes;
ordering the locks is still on you.

One warning about the GIL: it makes unlucky interleavings so rare that a
deadlock-prone solution can pass thousands of casual runs. The tests here
compensate by playing an adversarial scheduler — they wrap
`threading.Semaphore.acquire` with a forced nap so every window between
two fork pickups gets hit. Represent your forks as semaphores (the book's
representation) so that scrutiny lands on your code.

Run: `./check python 15`
