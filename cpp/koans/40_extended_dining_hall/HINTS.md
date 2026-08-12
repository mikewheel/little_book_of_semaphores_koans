# Hints — Koan 40: Extended Dining Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Mirror koan 39's leave-side machinery on the eat side: a third counter
`ready_to_eat_`, and a second parking semaphore `ok_to_sit_` (initially
0) for the student waiting with her tray.

The one eat-side blocking state: `eating_ == 0 && ready_to_eat_ == 1`
(that 1 is you). The only escape: a second ready-to-eat student arrives.

</details>

<details>
<summary>Hint 2</summary>

The symmetric case analysis after `get_food` (all under the mutex):

- `eating_ == 0 && ready_to_eat_ == 1` → release the mutex and wait on
  `ok_to_sit_`; your rescuer does your bookkeeping ("I'll do it for you").
- `eating_ == 0 && ready_to_eat_ == 2` → signal `ok_to_sit_`, move *both*
  of you from ready-to-eat to eating, and proceed.
- otherwise (someone is dining) → seat yourself immediately — and this
  is the branch that inherits koan 39's newcomer duty: if you now see
  `eating_ == 2 && ready_to_leave_ == 1`, free the stranded leaver.

The checkout is exactly koan 39's three cases, unchanged. Note the happy
fact from the book's analysis: a pair sitting down at an empty table
never needs to check for stranded leavers — nobody can be mid-meal at an
empty table.

</details>

<details>
<summary>Hint 3</summary>

Members: `std::mutex mutex_; std::counting_semaphore<> ok_to_sit_{0},
ok_to_leave_{0}; int ready_to_eat_ = 0, eating_ = 0, ready_to_leave_ = 0;`.

```cpp
void student(int sid, const std::function<void()>& dine_gate = {}) {
    hooks_.get_food(sid);

    mutex_.lock();
    ++ready_to_eat_;
    if (eating_ == 0 && ready_to_eat_ == 1) {
        mutex_.unlock();
        ok_to_sit_.acquire();             // wait for company
    } else if (eating_ == 0 && ready_to_eat_ == 2) {
        ok_to_sit_.release();             // we sit down together
        ready_to_eat_ -= 2;
        eating_ += 2;
        mutex_.unlock();
    } else {
        --ready_to_eat_;
        ++eating_;
        if (eating_ == 2 && ready_to_leave_ == 1) {
            ok_to_leave_.release();       // koan 39's newcomer duty
            --ready_to_leave_;
        }
        mutex_.unlock();
    }

    hooks_.dine(sid);
    if (dine_gate) dine_gate();

    mutex_.lock();
    --eating_;
    ++ready_to_leave_;
    if (eating_ == 1 && ready_to_leave_ == 1) {
        mutex_.unlock();
        ok_to_leave_.acquire();
    } else if (eating_ == 0 && ready_to_leave_ == 2) {
        ok_to_leave_.release();
        ready_to_leave_ -= 2;
        mutex_.unlock();
    } else {
        --ready_to_leave_;
        mutex_.unlock();
    }

    hooks_.leave(sid);
}
```

</details>
