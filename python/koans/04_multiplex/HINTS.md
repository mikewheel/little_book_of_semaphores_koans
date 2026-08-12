# Hints — Koan 04: Multiplex

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

In koan 03 the semaphore's initial value meant "how many may enter." You
set it to 1. What else could you set it to?

</details>

<details>
<summary>Hint 2</summary>

Think of the semaphore as a basket of `n` tokens. `enter()` takes one
(blocking if the basket is empty); `exit()` puts one back.

</details>

<details>
<summary>Hint 3</summary>

- `__init__`: `self.sem = threading.Semaphore(n)`
- `enter`: `self.sem.acquire()`
- `exit`: `self.sem.release()`

The mutex was never special — it's a multiplex with `n = 1`.

</details>
