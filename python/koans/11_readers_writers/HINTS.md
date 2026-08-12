# Hints — Koan 11: Readers-writers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Writers are easy: from a writer's point of view this is just a mutex over
"the room" — one semaphore, held from `writer_enter` to `writer_exit`.
The puzzle is the readers: they must hold that same "room" semaphore
*collectively* without each holding it individually. What bookkeeping
does that need? (A counter. Protected by what?)

</details>

<details>
<summary>Hint 2</summary>

The **Lightswitch** pattern: the *first* person into a room turns the
light on; the *last* one out turns it off. Keep a reader count under a
small mutex; the reader who bumps it 0→1 acquires the room semaphore, and
the reader who drops it 1→0 releases it. Everyone in between just walks
in. This pattern recurs in the next two koans — consider wrapping it in a
little helper class with `lock(semaphore)`/`unlock(semaphore)` methods
now, and reusing it later.

</details>

<details>
<summary>Hint 3</summary>

With `mutex = Lock()`, `room_empty = Semaphore(1)`, `readers = 0`:

```python
def reader_enter(self):
    with self.mutex:
        self.readers += 1
        if self.readers == 1:
            self.room_empty.acquire()   # first in locks out writers

def reader_exit(self):
    with self.mutex:
        self.readers -= 1
        if self.readers == 0:
            self.room_empty.release()   # last out lets writers back

def writer_enter(self):
    self.room_empty.acquire()

def writer_exit(self):
    self.room_empty.release()
```

Yes, the first reader may block on `room_empty` *while holding the
mutex* — that's intentional here: it makes later readers queue up behind
it until the writer leaves.

</details>
