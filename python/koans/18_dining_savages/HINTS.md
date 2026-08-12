# Hints — Koan 18: Dining Savages

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

You cannot peek at a semaphore's value, so a semaphore cannot *be* the
serving count — a diner has to know the pot is empty *before* blocking on
it. Keep the count on a scoreboard instead: a plain integer, guarded by a
mutex, that diners read and update themselves.

</details>

<details>
<summary>Hint 2</summary>

The roster: `servings = 0` (the scoreboard), a mutex protecting it, and two
signaling semaphores, `empty_pot = Semaphore(0)` and
`full_pot = Semaphore(0)`. A diner who finds `servings == 0` signals
`empty_pot` and waits on `full_pot` — *while still holding the mutex*.
That's normally a cardinal sin, but here it is safe: the cook is the only
other party, and the cook never touches the mutex. Holding it also does
double duty — it keeps every other diner out until the refill lands.

</details>

<details>
<summary>Hint 3</summary>

Both loops, in pseudocode:

```python
# cook (daemon)
while True:
    empty_pot.acquire()
    pot.put_servings(m)
    full_pot.release()

# diner
with mutex:
    if servings == 0:
        empty_pot.release()
        full_pot.acquire()
        servings = m
    servings -= 1
    pot.get_serving()
```

Note that the *diner* resets `servings = m` — that way every access to the
scoreboard is visibly inside the mutex. Keeping `pot.get_serving()` inside
the mutex is also what guarantees the pot calls never overlap.

</details>
