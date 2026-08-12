# Hints — Koan 01: Signaling

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

What does a semaphore initialized to **0** do to the first thread that
calls `acquire()` on it?

</details>

<details>
<summary>Hint 2</summary>

You need exactly one semaphore member. Give it a name that describes the
fact it announces: `a1_done`. Then `a1_done.release()` reads as "signal that
a1 is done" and `a1_done.acquire()` reads as "wait until a1 is done."

</details>

<details>
<summary>Hint 3</summary>

- Member: `std::counting_semaphore<> a1_done{0};` (or a
  `std::binary_semaphore a1_done{0};`).
- `run_a`: call `a1()`, then `a1_done.release();`
- `run_b`: `a1_done.acquire();` then call `b1()`.

Both orders work out: if A finishes first the semaphore holds the token and
B sails through; if B arrives first it blocks until A releases.

</details>
