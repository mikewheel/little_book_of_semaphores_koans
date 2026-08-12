# Hints — Koan 32: The sushi bar problem

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

A scoreboard: `eating` and `waiting` counters plus a `must_wait` flag,
all guarded by one mutex, and a `block = Semaphore(0)` for waiters to
park on. `must_wait` becomes true when `eating` hits `seats` and is the
*only* thing an arrival consults — not whether a seat happens to be free.

</details>

<details>
<summary>Hint 2</summary>

The dangerous moment is the wake-up. If a woken waiter re-acquires the
mutex to bump `eating` itself, fresh arrivals can beat it to the mutex
and take the seats first (the book shows this exact non-solution
over-filling the bar). Two etiquettes fix it:

- **"I'll do it for you"** — the *departing* customer, who already holds
  the mutex, moves the cohort's counts from `waiting` to `eating` and
  re-arms `must_wait` *before* releasing anyone. Woken waiters touch
  nothing; they just walk to their seats.
- **"Pass the baton"** — the signaler hands the mutex itself to the
  woken waiter (release the semaphore *instead of* the mutex); each
  waiter updates the state and passes the baton on, and the last one
  releases the mutex for real.

</details>

<details>
<summary>Hint 3</summary>

"I'll do it for you", in full (this is Reek's solution #1):

```python
def dine(self, eat):
    with self.mutex:
        if self.must_wait:
            self.waiting += 1
            wait_here = True
        else:
            self.eating += 1
            self.must_wait = (self.eating == self.seats)
            wait_here = False
    if wait_here:
        self.block.acquire()   # seat assigned by the last one out

    eat()

    with self.mutex:
        self.eating -= 1
        if self.eating == 0:
            n = min(self.seats, self.waiting)
            self.waiting -= n
            self.eating += n
            self.must_wait = (self.eating == self.seats)
            if n:
                self.block.release(n)   # the whole cohort at once
```

The departing thread seats the cohort while still holding the mutex, so
a newcomer that grabs the mutex next sees fully-updated state. Note the
re-armed `must_wait` when a 5-strong cohort refills the bar. Solution #2
("pass the baton") also works and needs no extra variables — try it as
an encore.

</details>
