# Hints — Koan 39: Dining Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard pattern: two counters, `eating_` and `ready_to_leave_`,
guarded by one `mutex_`, plus an `ok_to_leave_` semaphore (initially 0)
for the student who has to wait.

Enumerate the states a finishing student can see. There is exactly ONE
combination where she must wait: one other student still eating, and
nobody else ready to leave alongside her.

</details>

<details>
<summary>Hint 2</summary>

Both ways out of the sticky state use "I'll do it for you": whoever
changes the situation signals `ok_to_leave_` **and fixes the counters on
the waiter's behalf**, so the woken student never touches the mutex again
— she just walks out.

- A newcomer who sits down and sees `eating_ == 2 && ready_to_leave_ == 1`
  frees the waiter (and decrements `ready_to_leave_` for her).
- A finisher who sees `eating_ == 0 && ready_to_leave_ == 2` frees the
  waiter and zeroes the count for both of them — they leave together.

</details>

<details>
<summary>Hint 3</summary>

Members: `std::mutex mutex_; std::counting_semaphore<> ok_to_leave_{0};
int eating_ = 0, ready_to_leave_ = 0;`.

```cpp
void student(int sid, const std::function<void()>& dine_gate = {}) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++eating_;
        if (eating_ == 2 && ready_to_leave_ == 1) {
            ok_to_leave_.release();   // free the stranded waiter
            --ready_to_leave_;        // ...and do her bookkeeping
        }
    }
    hooks_.dine(sid);
    if (dine_gate) dine_gate();

    mutex_.lock();
    --eating_;
    ++ready_to_leave_;
    if (eating_ == 1 && ready_to_leave_ == 1) {
        mutex_.unlock();
        ok_to_leave_.acquire();       // the one blocking case
    } else if (eating_ == 0 && ready_to_leave_ == 2) {
        ok_to_leave_.release();       // we leave together
        ready_to_leave_ -= 2;
        mutex_.unlock();
    } else {
        --ready_to_leave_;
        mutex_.unlock();
    }
    hooks_.leave(sid);
}
```

The checkout side uses bare `lock()`/`unlock()` because the blocking
branch must release the mutex before sleeping — a scope-bound
`lock_guard` can't hand the lock away mid-scope. (`unique_lock` +
`unlock()` is the RAII-friendlier spelling.)

</details>
