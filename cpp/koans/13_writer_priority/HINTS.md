# Hints — Koan 13: Writer-priority readers-writers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 11 taught readers to hold a semaphore *collectively*: first one in
claims it, last one out returns it (the Lightswitch). The move here is
realizing the **writers need a Lightswitch too** — the first queued
writer bars the readers, and only the last writer of the convoy lets
them back. If you wrapped the pattern in a struct back in koan 11, this
koan is two instances of it.

</details>

<details>
<summary>Hint 2</summary>

Two condition semaphores, each guarded by one category's lightswitch:

- `no_readers` — held while any *writers* are queued or writing. Readers
  briefly touch it on the way in; writers hold it collectively for the
  whole convoy.
- `no_writers` — held while any *readers* are inside. Readers hold it
  collectively; each writer acquires it for the duration of its write.

A reader's entry is: pass through `no_readers` (acquire, then release
right away), switching the readers' lightswitch on `no_writers` while
inside. A writer's entry is: switch the writers' lightswitch on
`no_readers` (first writer blocks new readers), then acquire
`no_writers` for itself.

</details>

<details>
<summary>Hint 3</summary>

With a `Lightswitch` struct (counter + mutex +
`lock(std::binary_semaphore&)`/`unlock(std::binary_semaphore&)`) and
`std::binary_semaphore no_readers_{1}, no_writers_{1};`:

```cpp
void reader_enter() {
    no_readers_.acquire();
    read_switch_.lock(no_writers_);
    no_readers_.release();
}

void reader_exit() { read_switch_.unlock(no_writers_); }

void writer_enter() {
    write_switch_.lock(no_readers_);
    no_writers_.acquire();
}

void writer_exit() {
    no_writers_.release();
    write_switch_.unlock(no_readers_);
}
```

Why readers can't jump a convoy: writer #1 locks `no_readers_` via the
write switch and keeps it locked while writer #2, #3, … queue on
`no_writers_`; the switch only unlocks when the *last* of them exits.
Readers meanwhile pile up on `no_readers_`. Writers pass the room among
themselves via `no_writers_`, never releasing `no_readers_` in between.

</details>
