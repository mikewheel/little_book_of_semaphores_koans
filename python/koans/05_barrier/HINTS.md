# Hints — Koan 05: Barrier

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Ingredients: `count = 0` (arrivals so far), a mutex protecting it, and a
semaphore initialized to 0 for early arrivals to sleep on. Count your own
arrival inside the mutex; if you're the `n`th, do something about it.

</details>

<details>
<summary>Hint 2</summary>

The `n`th thread signals the sleep semaphore — but one `release()` wakes
only ONE sleeper. Two classic fixes:

- **Turnstile**: every thread does `acquire()` *immediately followed by*
  `release()`. Each thread woken passes the wake along to the next; the
  door stays propped open.
- **Preload**: the `n`th thread calls `release(n)` in one go.

Also: make sure nobody sleeps on the semaphore *while holding the mutex*,
or the count can never reach `n`.

</details>

<details>
<summary>Hint 3</summary>

Turnstile version of `wait()`:

```python
with self.mutex:            # a Semaphore(1) or your koan-03 Mutex
    self.count += 1
    is_last = self.count == self.n
if is_last:
    self.turnstile.release()
self.turnstile.acquire()    # sleep here until the door opens…
self.turnstile.release()    # …then hold it open for the next thread
```

with `self.turnstile = threading.Semaphore(0)`. After all `n` pass, the
turnstile ends at value 1, not 0 — one reason this barrier is single-use.
Koan 06 makes you fix that.

</details>
