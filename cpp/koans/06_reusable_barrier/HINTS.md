# Hints — Koan 06: Reusable barrier

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two questions to sit with. First: what state has to be back to its
initial value before the next round can start? (The arrival count, and
the door itself.) Second: *when* can you safely reset it? If the door is
still open while stragglers are leaving, a fast thread can loop around,
re-enter, and pass through with the previous cohort — it "laps" the
field. Whatever you design must make that impossible even under a
vicious scheduler, and one turnstile is not enough.

</details>

<details>
<summary>Hint 2</summary>

Use **two turnstiles that alternate**: while one is open the other is
locked. Arrivals sleep on the first turnstile; the `n`th arrival opens
it. Departures sleep on the second; the last one out opens that. A fast
thread looping back to `phase1()` slams into the first turnstile, which
nobody has reopened yet — lapping is now structurally impossible. Keep
the counter checks *inside* the mutex, or two threads can both decide
they were the `n`th.

</details>

<details>
<summary>Hint 3</summary>

The tidy "preloaded" version: the opener issues exactly `n` passes at
once, so the last thread through consumes the final token and the door
is already shut — no separate relocking step.

```cpp
void phase1() {
    {
        std::lock_guard lock(mutex_);
        if (++count_ == n_) turnstile1_.release(n_);  // n passes at once
    }
    turnstile1_.acquire();
}

void phase2() {
    {
        std::lock_guard lock(mutex_);
        if (--count_ == 0) turnstile2_.release(n_);
    }
    turnstile2_.acquire();
}
```

with members `std::mutex mutex_; int count_ = 0;
std::counting_semaphore<> turnstile1_{0}, turnstile2_{0};`.

The classic non-preloaded variant instead starts `turnstile2_` at 1; the
`n`th arrival does `turnstile2_.acquire()` then `turnstile1_.release()`
(lock the exit before opening the entrance), the last leaver does the
mirror image, and every thread files through each turnstile with an
`acquire()` immediately followed by a `release()`.

</details>
