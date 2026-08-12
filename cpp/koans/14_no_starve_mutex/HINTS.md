# Hints — Koan 14: No-starve mutex

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

You cannot assume any queue order, so stop thinking in queues. Think in
**phases**. Suppose entry to the critical section happens in batches: for a
while, arriving threads may check in (phase one); then check-in closes and
everyone already checked in gets served, one at a time, before check-in
reopens (phase two). A thread can then be overtaken at most by the current
batch plus the one batch that forms while it waits — bounded, no matter
how maliciously the semaphore picks winners.

Two rooms, two doors. While the door into room 1 is open, the door out of
room 2 is shut, and vice versa.

</details>

<details>
<summary>Hint 2</summary>

Morris's member roster (all `WeakSemaphore` + ints):

- `int room1_ = 0, room2_ = 0;` — plain headcounts of the two waiting
  rooms.
- `WeakSemaphore mutex_{1};` — guards the `room1_` counter.
- `WeakSemaphore t1_{1};` — turnstile between room 1 and room 2. Open
  initially.
- `WeakSemaphore t2_{0};` — turnstile between room 2 and the critical
  section. Closed initially.

The trick is who signals which turnstile, and how the counters decide it:
the *last* thread out of room 1 is the one that switches the system into
phase two, and the *last* thread out of room 2 switches it back. Exactly
one of `t1_`/`t2_` ever carries a token, so holding "a turnstile token" is
what makes the critical section exclusive.

</details>

<details>
<summary>Hint 3</summary>

Morris's algorithm, in full:

```cpp
void acquire() {
    mutex_.acquire();
    ++room1_;
    mutex_.release();

    t1_.acquire();
    ++room2_;
    mutex_.acquire();
    --room1_;
    if (room1_ == 0) {
        mutex_.release();
        t2_.release();   // room 1 drained: open the inner door
    } else {
        mutex_.release();
        t1_.release();   // keep the procession through room 1 going
    }

    t2_.acquire();
    --room2_;
    // ...critical section runs after acquire() returns...
}

void release() {
    if (room2_ == 0)
        t1_.release();   // batch served: reopen the outer door
    else
        t2_.release();   // let the next of the batch through
}
```

Why it bounds overtaking: while you wait at `t1_`, every thread that beats
you through it gets parked at `t2_` (closed), so each peer passes at most
once before the phase flips; while you wait at `t2_`, no newcomer can even
reach it because `t1_` is closed. `room2_` needs no mutex: it is only
touched by a thread holding `t1_`-or-`t2_` exclusivity, and every handoff
threads through a `WeakSemaphore`'s internal lock, so the compiler sees
the ordering too. Follow one thread through alone, then two, and watch
the token bounce between `t1_` and `t2_` — there is always exactly one
token total.

</details>
