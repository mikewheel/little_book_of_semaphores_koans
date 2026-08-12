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

The doorway is a **turnstile**: a `std::binary_semaphore{1}` that readers
acquire and *immediately* release on their way in. A writer acquires it
and does **not** release it until it's done — it waits for the room to
empty while standing inside the turnstile. Effect: the moment a writer
queues up, every later arrival (reader or writer) piles up at the
turnstile; when the last incumbent reader leaves, the writer is the only
thread that can possibly go next. Keep koan 11's machinery (reader count
+ mutex + `room_empty` — a Lightswitch, if you built one) for the room
itself.

</details>

<details>
<summary>Hint 3</summary>

With `std::binary_semaphore turnstile_{1};` added to koan 11's members:

```cpp
void reader_enter() {
    turnstile_.acquire();
    turnstile_.release();          // pass straight through…
    std::lock_guard lock(mutex_);  // …then koan 11's entry
    if (++readers_ == 1) room_empty_.acquire();
}

void writer_enter() {
    turnstile_.acquire();          // block the doorway…
    room_empty_.acquire();         // …and wait for the room, inside it
}

void writer_exit() {
    turnstile_.release();          // reopen the doorway
    room_empty_.release();
}
```

`reader_exit` is unchanged from koan 11. The writer releases the
turnstile on exit, so a whole crowd (readers and writers alike) can queue
during one write — but nobody new can slip in during the wait.

</details>
