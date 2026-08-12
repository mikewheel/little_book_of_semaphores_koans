# Hints — Koan 24: River crossing

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard again: counters `hackers_` and `serfs_` under one semaphore-
as-mutex, plus two queues (a semaphore hackers sleep on, one serfs sleep
on). Each arrival bumps its counter and asks: did I just complete a crew?
Three completing conditions exist — 4 hackers, 4 serfs, or 2+2. The
completer releases the right mix of queue tokens (including one for
itself), fixes the counters, and remembers *in a stack-local variable*
that it is the captain.

</details>

<details>
<summary>Hint 2</summary>

Two more pieces:

- The completer does **not** release the mutex — it keeps the dock closed
  so no fifth passenger can slip into this boatload. It releases the mutex
  only after the boat has sailed.
- All four crew members meet at a reusable **barrier of size 4** after
  boarding (koan 06, or `std::barrier<>`). Once the barrier opens,
  everyone knows all four boards happened; the captain — flagged by that
  local `is_captain` — rows and then reopens the dock.

Both arrival methods are the same algorithm with the roles swapped:
delegate to one private helper.

</details>

<details>
<summary>Hint 3</summary>

```cpp
void arrive(bool hacker) {
    bool is_captain = false;        // stack local: private to this call
    const std::string kind = hacker ? "hacker" : "serf";
    mutex_.acquire();               // std::binary_semaphore{1}
    (hacker ? hackers_ : serfs_) += 1;
    if (hackers_ == 4) {
        hacker_queue_.release(4); hackers_ = 0; is_captain = true;
    } else if (serfs_ == 4) {
        serf_queue_.release(4); serfs_ = 0; is_captain = true;
    } else if (hackers_ >= 2 && serfs_ >= 2) {
        hacker_queue_.release(2); serf_queue_.release(2);
        hackers_ -= 2; serfs_ -= 2; is_captain = true;
    } else {
        mutex_.release();           // incomplete crew: step back out
    }
    (hacker ? hacker_queue_ : serf_queue_).acquire();
    hooks_.board(kind);
    barrier_.arrive_and_wait();     // std::barrier<> barrier_{4}
    if (is_captain) {
        hooks_.row_boat(kind);
        mutex_.release();           // boat has sailed: reopen the dock
    }
}
```

The captain holds the "mutex" from crew completion until after
`row_boat` — that hold is what keeps boatloads from interleaving, and why
it must be a semaphore rather than a `std::mutex`.

</details>
