# Hints — Koan 11: Readers-writers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Writers are easy: from a writer's point of view this is just a mutex over
"the room" — one `std::binary_semaphore`, held from `writer_enter` to
`writer_exit`. The puzzle is the readers: they must hold that same "room"
semaphore *collectively* without each holding it individually. What
bookkeeping does that need? (A counter. Protected by what?)

</details>

<details>
<summary>Hint 2</summary>

The **Lightswitch** pattern: the *first* person into a room turns the
light on; the *last* one out turns it off. Keep a reader count under a
small `std::mutex`; the reader who bumps it 0→1 acquires the room
semaphore, and the reader who drops it 1→0 releases it. Everyone in
between just walks in. This pattern recurs in the next two koans —
consider wrapping it in a little helper struct with
`lock(binary_semaphore&)`/`unlock(binary_semaphore&)` methods now, and
reusing it later.

</details>

<details>
<summary>Hint 3</summary>

With `std::mutex mutex_;`, `std::binary_semaphore room_empty_{1};`,
`int readers_ = 0;`:

```cpp
void reader_enter() {
    std::lock_guard lock(mutex_);
    if (++readers_ == 1)
        room_empty_.acquire();   // first in locks out writers
}

void reader_exit() {
    std::lock_guard lock(mutex_);
    if (--readers_ == 0)
        room_empty_.release();   // last out lets writers back
}

void writer_enter() { room_empty_.acquire(); }
void writer_exit() { room_empty_.release(); }
```

Yes, the first reader may block on `room_empty_` *while holding the
mutex* — that's intentional here: it makes later readers queue up behind
it until the writer leaves.

</details>
