# Hints — Koan 12: No-starve readers-writers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

You can't stop the readers already inside — rule 3 says they finish
normally. What you need is a *doorway in front of the whole lock* that a
waiting writer can jam shut behind itself. Readers walk through the
doorway when it's clear; nobody passes while a writer stands in it.

</details>

<details>
<summary>Hint 2</summary>

The doorway is a **turnstile**: a `Semaphore(1)` that readers acquire and
*immediately* release on their way in. A writer acquires it and does
**not** release it until it's done — it waits for the room to empty while
standing inside the turnstile. Effect: the moment a writer queues up,
every later arrival (reader or writer) piles up at the turnstile; when
the last incumbent reader leaves, the writer is the only thread that can
possibly go next. Keep koan 11's machinery (reader count + mutex +
`room_empty` — a Lightswitch, if you built one) for the room itself.

</details>

<details>
<summary>Hint 3</summary>

With `turnstile = Semaphore(1)` added to koan 11's members:

```python
def reader_enter(self):
    self.turnstile.acquire()
    self.turnstile.release()      # pass straight through…
    with self.mutex:              # …then koan 11's entry
        self.readers += 1
        if self.readers == 1:
            self.room_empty.acquire()

def writer_enter(self):
    self.turnstile.acquire()      # block the doorway…
    self.room_empty.acquire()     # …and wait for the room, still inside it

def writer_exit(self):
    self.turnstile.release()      # reopen the doorway
    self.room_empty.release()
```

`reader_exit` is unchanged from koan 11. The writer releases the
turnstile on exit, so a whole crowd (readers and writers alike) can queue
during one write — but nobody new can slip in during the wait.

</details>
