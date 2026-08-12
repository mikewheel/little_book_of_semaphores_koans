# Hints — Koan 25: Roller coaster

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two queue semaphores carry the car's permissions to the passengers:
`board_queue_` ("you may board") and `unboard_queue_` ("you may
unboard"), both starting at 0. After `load()`, the car releases
`board_queue_` **capacity times in one call**; after `unload()`, same for
`unboard_queue_`. Each passenger acquires one token from each queue at
the right moment.

</details>

<details>
<summary>Hint 2</summary>

The reverse direction is fan-in: the car must learn that all `capacity`
passengers have boarded (and later, unboarded). Give the passengers a
counter protected by a `std::mutex`; whoever increments it to `capacity`
signals a semaphore the car is waiting on (`all_aboard_`, plus a second
pair `all_ashore_` for the way out) and resets the counter to zero — the
last passenger resets it, nobody else, while still holding the mutex.
Use separate mutex+counter pairs for boarding and unboarding.

</details>

<details>
<summary>Hint 3</summary>

```cpp
void start_car(int n_rides) {
    for (int r = 0; r < n_rides; ++r) {
        hooks_.load();
        board_queue_.release(capacity_);
        all_aboard_.acquire();
        hooks_.run();
        hooks_.unload();
        unboard_queue_.release(capacity_);
        all_ashore_.acquire();
    }
}

void passenger(int pid) {
    board_queue_.acquire();
    hooks_.board(pid);
    {
        std::lock_guard lock(board_mutex_);
        if (++boarders_ == capacity_) { all_aboard_.release(); boarders_ = 0; }
    }
    unboard_queue_.acquire();
    hooks_.unboard(pid);
    {
        std::lock_guard lock(unboard_mutex_);
        if (++unboarders_ == capacity_) { all_ashore_.release(); unboarders_ = 0; }
    }
}
```

The tokens released in cycle *k* are all consumed in cycle *k* (exactly
`capacity` of each), so the queues are safely reusable.

</details>
