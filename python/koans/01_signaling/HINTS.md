# Hints — Koan 01: Signaling

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

What does a semaphore initialized to **0** do to the first thread that
calls `acquire()` on it?

</details>

<details>
<summary>Hint 2</summary>

You need exactly one semaphore. Give it a name that describes the fact it
announces, e.g. `a1_done`. Then `a1_done.release()` reads as "signal that a1
is done" and `a1_done.acquire()` reads as "wait until a1 is done."

</details>

<details>
<summary>Hint 3</summary>

- `__init__`: `self.a1_done = threading.Semaphore(0)`
- `run_a`: run `a1()`, then release the semaphore.
- `run_b`: acquire the semaphore, then run `b1()`.

Both orders work out: if A finishes first the semaphore holds the token and
B sails through; if B arrives first it blocks until A releases.

</details>
