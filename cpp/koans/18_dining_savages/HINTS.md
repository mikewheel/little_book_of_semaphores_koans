# Hints — Koan 18: Dining Savages

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

You cannot peek at a semaphore's value, so a semaphore cannot *be* the
serving count — a diner has to know the pot is empty *before* blocking on
it. Keep the count on a scoreboard instead: a plain `int`, guarded by a
`std::mutex`, that diners read and update themselves.

</details>

<details>
<summary>Hint 2</summary>

The roster: `int servings = 0` (the scoreboard), a `std::mutex` protecting
it, and two signaling semaphores initialized to 0 — `empty_pot` and
`full_pot`. A diner who finds `servings == 0` signals `empty_pot` and waits
on `full_pot` — *while still holding the mutex*. That's normally a cardinal
sin, but here it is safe: the cook is the only other party, and the cook
never touches the mutex. Holding it also does double duty — it keeps every
other diner out until the refill lands.

</details>

<details>
<summary>Hint 3</summary>

Both loops, in pseudocode:

```cpp
// cook — a detached thread started by start_cook()
while (true) {
    empty_pot.acquire();
    pot_.put_servings(m_);
    full_pot.release();
}

// dine()
std::lock_guard lock(mutex_);
if (servings_ == 0) {
    empty_pot.release();
    full_pot.acquire();
    servings_ = m_;
}
--servings_;
pot_.get_serving();
```

Note that the *diner* resets `servings_ = m_` — that way every access to
the scoreboard is visibly inside the mutex. Keeping `pot_.get_serving()`
inside the mutex is also what guarantees the pot calls never overlap. The
cook's lambda captures `this`; the Village must outlive the detached
thread (the tests arrange that).

</details>
