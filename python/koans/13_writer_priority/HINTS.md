# Hints — Koan 13: Writer-priority readers-writers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 11 taught readers to hold a semaphore *collectively*: first one in
claims it, last one out returns it (the Lightswitch). The move here is
realizing the **writers need a Lightswitch too** — the first queued
writer bars the readers, and only the last writer of the convoy lets
them back. If you wrapped the pattern in a class back in koan 11, this
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

With `read_switch`/`write_switch` as Lightswitches and
`no_readers = Semaphore(1)`, `no_writers = Semaphore(1)`:

```python
def reader_enter(self):
    self.no_readers.acquire()
    self.read_switch.lock(self.no_writers)
    self.no_readers.release()

def reader_exit(self):
    self.read_switch.unlock(self.no_writers)

def writer_enter(self):
    self.write_switch.lock(self.no_readers)
    self.no_writers.acquire()

def writer_exit(self):
    self.no_writers.release()
    self.write_switch.unlock(self.no_readers)
```

Why readers can't jump a convoy: writer #1 locks `no_readers` via the
write switch and keeps it locked while writer #2, #3, … queue on
`no_writers`; the switch only unlocks when the *last* of them exits.
Readers meanwhile pile up on `no_readers`. Writers pass the room among
themselves via `no_writers`, never releasing `no_readers` in between.

</details>
