# Hints — Koan 34: Extended child care

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Stop thinking in tokens. A departing adult that pre-claims capacity is the
very bug you're asked to remove. Instead, keep an explicit scoreboard under
one mutex: counts of `children` and `adults` inside, plus `waiting`
(children at the door) and `leaving` (adults at the door). Every decision —
may this child enter? may this adult go? — is a comparison against the
scoreboard, made while holding the mutex.

</details>

<details>
<summary>Hint 2</summary>

Who wakes the sleepers? Not themselves — a semaphore waiter can't re-check
the scoreboard atomically on wake. Use "I'll do it for you": the thread
whose action changes the counts also hands out the consequences, while it
still holds the mutex.

- `child_leave` decrements `children`, then checks whether some adult in
  `leaving` can now go — if so it updates the scoreboard *on the leaver's
  behalf* and signals the adult queue.
- `adult_enter` increments `adults`, then admits up to `ratio` waiting
  children the same way: it adjusts `waiting`/`children` itself and signals
  the child queue that many times.

A woken thread therefore wakes with its state transition already done, and
just walks through.

</details>

<details>
<summary>Hint 3</summary>

```text
mutex; children = adults = waiting = leaving = 0
child_queue = Semaphore(0); adult_queue = Semaphore(0)

child_enter:  with mutex:
                  if children < ratio * adults: children++; return
                  waiting++
              child_queue.acquire()          # bookkeeping already done

child_leave:  with mutex:
                  children--
                  if leaving > 0 and children <= ratio * (adults - 1):
                      leaving--; adults--; adult_queue.release()

adult_enter:  with mutex:
                  adults++
                  n = min(ratio, waiting)
                  waiting -= n; children += n
                  child_queue.release(n)

adult_leave:  with mutex:
                  if children <= ratio * (adults - 1): adults--; return
                  leaving++
              adult_queue.acquire()          # bookkeeping already done
```

An adult in `leaving` still counts in `adults`, which is exactly why a new
child may enter past a stuck leaver.

</details>
