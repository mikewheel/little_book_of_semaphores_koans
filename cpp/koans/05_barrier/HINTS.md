# Hints — Koan 05: Barrier

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Ingredients: an `int count_ = 0` (arrivals so far), a mutex protecting it,
and a `std::counting_semaphore<> turnstile_{0}` for early arrivals to sleep
on. Count your own arrival under the mutex; if you're the `n`th, do
something about it.

</details>

<details>
<summary>Hint 2</summary>

The `n`th thread releases the sleep semaphore — but one `release()` wakes
only ONE sleeper. Two classic fixes:

- **Turnstile**: every thread does `acquire()` *immediately followed by*
  `release()`. Each woken thread passes the wake along; the door stays
  propped open.
- **Preload**: the `n`th thread calls `release(n)` in one go.

Also: make sure nobody sleeps on the semaphore *while holding the mutex*,
or the count can never reach `n`.

</details>

<details>
<summary>Hint 3</summary>

Turnstile version of `wait()`:

```cpp
{
    std::lock_guard lock(mutex_);
    ++count_;
    if (count_ == n_) turnstile_.release();
}
turnstile_.acquire();   // sleep here until the door opens…
turnstile_.release();   // …then hold it open for the next thread
```

with members `std::mutex mutex_; int count_ = 0;
std::counting_semaphore<> turnstile_{0};`. After all `n` pass, the
turnstile ends at value 1, not 0 — one reason this barrier is single-use.
Koan 06 makes you fix that.

</details>
