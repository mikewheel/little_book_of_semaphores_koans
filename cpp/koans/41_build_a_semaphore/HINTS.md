# Hints — Koan 41: Build a semaphore

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

The book's ingredient list: an integer `value_`, a second integer
`wakeups_`, one `std::mutex`, one `std::condition_variable` — the mutex
guards both counters.

`value_` may go negative: a negative value counts how many threads are
waiting. `wakeups_` counts signals that have been sent but not yet
consumed by a woken thread.

</details>

<details>
<summary>Hint 2</summary>

Why `wakeups_` exists: with only `value_` and a
`while (value_ <= 0) cond_.wait(lock);` loop, a releaser can signal and
then *race back around* and call `acquire()` before the sleeper resumes —
the fresh caller sees a positive value and takes it, and the sleeper
wakes to find nothing. The signal was meant for a *waiter* (the book's
Property 3), so the woken thread must consume a `wakeups_` token that
fresh arrivals never touch.

The book writes this as a do-while: sleep *first*, then check —

```cpp
do {
    cond_.wait(lock);
} while (wakeups_ < 1);
--wakeups_;
```

The re-loop also absorbs spurious wakeups, which the C++ standard
explicitly allows — one more reason condition waits are always loops.

</details>

<details>
<summary>Hint 3</summary>

```cpp
void acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    --value_;
    if (value_ < 0) {
        do {
            cond_.wait(lock);
        } while (wakeups_ < 1);
        --wakeups_;
    }
}

void release() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++value_;
    if (value_ <= 0) {           // someone is waiting
        ++wakeups_;
        cond_.notify_one();
    }
}
```

Members: `std::mutex mutex_; std::condition_variable cond_;
int value_; int wakeups_ = 0;`. Note that `release()` only notifies when
the value was negative — a banked permit with nobody waiting needs no
wakeup, which is exactly Property 2 falling out of the arithmetic. And
`acquire()` needs `unique_lock`, not `lock_guard`: `wait()` must be able
to unlock and relock it.

</details>
