# Hints — Koan 32: The sushi bar problem

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

A scoreboard: `eating` and `waiting` counters plus a `must_wait` flag,
all guarded by one `std::mutex`, and a `std::counting_semaphore<>
block{0}` for waiters to park on. `must_wait` becomes true when `eating`
hits `seats` and is the *only* thing an arrival consults — not whether a
seat happens to be free.

</details>

<details>
<summary>Hint 2</summary>

The dangerous moment is the wake-up. If a woken waiter re-locks the
mutex to bump `eating` itself, fresh arrivals can beat it to the mutex
and take the seats first (the book shows this exact non-solution
over-filling the bar). Two etiquettes fix it:

- **"I'll do it for you"** — the *departing* customer, who already holds
  the mutex, moves the cohort's counts from `waiting` to `eating` and
  re-arms `must_wait` *before* `block.release(n)`. Woken waiters touch
  nothing; they just walk to their seats.
- **"Pass the baton"** — the signaler hands the critical section itself
  to the woken waiter. With semaphores this is legal; with `std::mutex`
  it is UB (unlock by a non-owner). In modern C++ you express the baton
  as a `std::condition_variable` predicate instead.

</details>

<details>
<summary>Hint 3</summary>

"I'll do it for you", in full (Reek's solution #1):

```cpp
void dine(const std::function<void()>& eat) {
    bool wait_here;
    {
        std::lock_guard lock(mutex_);
        if (must_wait_) {
            ++waiting_;
            wait_here = true;
        } else {
            ++eating_;
            must_wait_ = (eating_ == seats_);
            wait_here = false;
        }
    }
    if (wait_here) block_.acquire();  // seat assigned by the last one out

    eat();

    {
        std::lock_guard lock(mutex_);
        --eating_;
        if (eating_ == 0) {
            int n = std::min(seats_, waiting_);
            waiting_ -= n;
            eating_ += n;
            must_wait_ = (eating_ == seats_);
            if (n > 0) block_.release(n);
        }
    }
}
```

The departing thread seats the cohort while still holding the mutex, so
a newcomer that locks the mutex next sees fully-updated state. Note the
re-armed `must_wait` when a 5-strong cohort refills the bar. Reek's
solution #2 ("pass the baton") also works — with a semaphore as the
lock, or idiomatically as a `condition_variable` predicate — and needs
no extra variables.

</details>
