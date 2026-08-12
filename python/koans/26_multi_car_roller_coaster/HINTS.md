# Hints — Koan 26: Multi-car roller coaster

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Keep the whole koan-25 machinery (two queues, two counter+mutex pairs,
`all_aboard`, `all_ashore`) — it is unchanged. The new ingredient: two
**rings of turn semaphores**, one for the loading area and one for the
unloading area. Each ring has one semaphore per car; exactly one semaphore
per ring starts at 1 (car 0's) and the rest at 0. A car may only enter an
area after acquiring its own turn semaphore.

</details>

<details>
<summary>Hint 2</summary>

Define `next(i) = (i + 1) % n_cars`. When car `i` finishes with an area,
it releases the semaphore of car `next(i)` in that ring — passing the
baton. Loading turn is surrendered as soon as the car is full (right
after `all_aboard`), *before* running, so the next car can board while
this one is on the track. Unloading turn is surrendered after
`all_ashore`. Because both rings start at car 0 and pass in the same
order, unloads automatically happen in load order.

</details>

<details>
<summary>Hint 3</summary>

Car `i`'s cycle wraps the koan-25 car body in the two rings:

```python
loading_area[i].acquire()
self.hooks.load(i)
board_queue.release(capacity)
all_aboard.acquire()
loading_area[next(i)].release()   # dock is free: next car may load

self.hooks.run(i)

unloading_area[i].acquire()
self.hooks.unload(i)
unboard_queue.release(capacity)
all_ashore.acquire()
unloading_area[next(i)].release() # platform free: next car may unload
```

The passenger code is byte-for-byte the koan-25 passenger.
`start_cars` spawns one thread per car running this loop
`n_rides_per_car` times, then joins them all.

```python
self.loading_area = [
    threading.Semaphore(1 if i == 0 else 0) for i in range(n_cars)
]
```

</details>
