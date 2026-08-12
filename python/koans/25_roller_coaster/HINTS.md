# Hints — Koan 25: Roller coaster

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two queue semaphores carry the car's permissions to the passengers:
`board_queue` ("you may board") and `unboard_queue` ("you may unboard"),
both starting at 0. After `load()`, the car releases `board_queue`
**capacity times in one go**; after `unload()`, same for `unboard_queue`.
Each passenger acquires one token from each queue at the right moment.

</details>

<details>
<summary>Hint 2</summary>

The reverse direction is fan-in: the car must learn that all `capacity`
passengers have boarded (and later, unboarded). Give the passengers a
counter protected by a mutex; whoever increments it to `capacity` signals
a semaphore the car is waiting on (`all_aboard`, and a second pair
`all_ashore` for the way out) and resets the counter to zero — the last
passenger resets it, nobody else. Use separate mutex+counter pairs for
boarding and unboarding; they overlap in time across cycles.

</details>

<details>
<summary>Hint 3</summary>

```python
# car, per cycle                    # passenger, one ride
load()                              board_queue.acquire()
board_queue.release(capacity)       board(pid)
all_aboard.acquire()                with mutex:
run()                                   boarders += 1
unload()                                if boarders == capacity:
unboard_queue.release(capacity)             all_aboard.release()
all_ashore.acquire()                        boarders = 0
                                    unboard_queue.acquire()
                                    unboard(pid)
                                    with mutex2:
                                        unboarders += 1
                                        if unboarders == capacity:
                                            all_ashore.release()
                                            unboarders = 0
```

`start_car(n_rides)` wraps the left column in a `for` loop. The tokens
released in cycle *k* are all consumed in cycle *k* (exactly `capacity`
of each), so the queues are safely reusable.

</details>
