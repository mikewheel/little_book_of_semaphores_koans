# Hints — Koan 03: Mutex

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Think of the semaphore's value as "how many threads may enter right now."
How many should that be, before anyone has acquired?

</details>

<details>
<summary>Hint 2</summary>

Initialize the semaphore to **1**. The first `acquire()` takes the token
(value drops to 0), so the next one blocks. `release()` returns the token.

</details>

<details>
<summary>Hint 3</summary>

- `__init__`: `self.sem = threading.Semaphore(1)`
- `acquire`: `self.sem.acquire()`
- `release`: `self.sem.release()`

That's the entire koan: *mutex = semaphore initialized to 1*. The insight
worth keeping is the symmetry argument — any number of threads can run the
same wait/update/signal code and no two are ever inside together.

</details>
