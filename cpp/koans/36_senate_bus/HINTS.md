# Hints — Koan 36: Senate bus

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Keep a `waiting` count under a mutex; riders increment it on arrival. The
cutoff falls out of a single decision: let the bus *hold that mutex for
the entire boarding process*. Anyone arriving mid-boarding blocks at the
increment and only joins the count after this bus has gone — they are, by
construction, next-bus riders.

</details>

<details>
<summary>Hint 2</summary>

Two book solutions, same scoreboard:

- **Pass the baton** (rider→rider→bus): the bus wakes the first rider and
  hands over; each boarded rider wakes the next; the last one wakes the
  bus. Correct, but the handoff chain is fiddly.
- **"I'll do it for you"** (simpler — recommended): the bus computes
  `n = min(waiting, capacity)` once, then loops `n` times doing
  signal(rider-may-board) / wait(rider-has-boarded), then subtracts `n`
  from `waiting` itself. The per-iteration handshake stops the bus from
  handing out more wakeups than riders have consumed, and `n` — computed
  before any newcomer can slip in — *is* the cutoff.

</details>

<details>
<summary>Hint 3</summary>

```text
waiting = 0; mutex
bus     = counting_semaphore<>(0)   // a boarding turn is open
boarded = counting_semaphore<>(0)   // a rider finished boarding

rider(rid):
    { lock_guard g(mutex); ++waiting; }
    bus.acquire();       // wait for a bus to call on me
    board(rid);
    boarded.release();

bus_arrives():
    int n;
    { lock_guard g(mutex);          // doors of the *count* close here
      n = min(waiting, capacity);
      for i in 0..n: { bus.release(); boarded.acquire(); }
      waiting -= n; }
    depart(n);
```

Riders who arrive during boarding sit in the mutex; they increment
`waiting` only after this bus has departed, so the next bus counts them.
The snapshot `n` also caps boarding at `capacity` without any extra
machinery.

</details>
