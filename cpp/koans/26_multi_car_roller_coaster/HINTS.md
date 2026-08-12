# Hints — Koan 26: Multi-car roller coaster

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Keep the whole koan-25 machinery (two queues, two counter+mutex pairs,
`all_aboard_`, `all_ashore_`) — it is unchanged. The new ingredient: two
**rings of turn semaphores**, one for the loading area and one for the
unloading area. Each ring has one semaphore per car; exactly one semaphore
per ring starts at 1 (car 0's) and the rest at 0. A car may only enter an
area after acquiring its own turn semaphore. (Remember the README's note
about `std::vector` of semaphores before you write the member.)

</details>

<details>
<summary>Hint 2</summary>

Define `next(i) = (i + 1) % n_cars`. When car `i` finishes with an area,
it releases the semaphore of car `next(i)` in that ring — passing the
baton. Loading turn is surrendered as soon as the car is full (right after
`all_aboard_`), *before* running, so the next car can board while this one
is on the track. Unloading turn is surrendered after `all_ashore_`.
Because both rings start at car 0 and pass in the same order, unloads
automatically happen in load order.

</details>

<details>
<summary>Hint 3</summary>

Car `i`'s cycle wraps the koan-25 car body in the two rings:

```cpp
void car(int i, int n_rides) {
    for (int r = 0; r < n_rides; ++r) {
        loading_area_[i].acquire();
        hooks_.load(i);
        board_queue_.release(capacity_);
        all_aboard_.acquire();
        loading_area_[next(i)].release();    // dock free: next car loads

        hooks_.run(i);

        unloading_area_[i].acquire();
        hooks_.unload(i);
        unboard_queue_.release(capacity_);
        all_ashore_.acquire();
        unloading_area_[next(i)].release();  // platform free
    }
}
```

Members and startup:

```cpp
std::deque<std::binary_semaphore> loading_area_;   // deque, not vector!
std::deque<std::binary_semaphore> unloading_area_;
// in the constructor:
for (int i = 0; i < n_cars; ++i) {
    loading_area_.emplace_back(i == 0 ? 1 : 0);
    unloading_area_.emplace_back(i == 0 ? 1 : 0);
}
```

`passenger` is byte-for-byte the koan-25 passenger. `start_cars` spawns
one thread per car running `car(i, n_rides_per_car)` and joins them all.

</details>
