# Hints — Koan 23: Building H₂O

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Use a scoreboard: counters `hydrogen_` and `oxygen_` protected by a mutex
(one with no ownership — see the README's UB note), plus two queues — a
semaphore hydrogens sleep on and a semaphore oxygens sleep on. Each
arrival takes the mutex, bumps its counter, and checks whether a full
molecule is now waiting (2 H and 1 O). If yes, it releases 2 tokens to the
hydrogen queue and 1 to the oxygen queue — including one for itself — and
decrements the counters. If no, it releases the mutex and sleeps on its
queue.

</details>

<details>
<summary>Hint 2</summary>

Counters alone let molecules smear together. After `bond`, make all three
threads meet at a reusable **barrier of size 3** (you built one in koan 06;
`std::barrier<>` with 3 parties also works). And notice: the thread that
completed the molecule *kept the mutex* — nobody released it in the
if-branch. It stays held until the whole molecule has bonded, barring the
next molecule from forming. Who releases it after the barrier? Any single
one of the three works, and exactly one of them is unique per molecule:
the oxygen does it.

</details>

<details>
<summary>Hint 3</summary>

```cpp
void hydrogen() {
    mutex_.acquire();                      // std::binary_semaphore{1}
    ++hydrogen_;
    if (hydrogen_ >= 2 && oxygen_ >= 1) {
        hydro_queue_.release(2);  hydrogen_ -= 2;
        oxy_queue_.release();     --oxygen_;
    } else {
        mutex_.release();                  // molecule incomplete: back out
    }
    hydro_queue_.acquire();
    hooks_.bond("H");
    barrier_.arrive_and_wait();            // std::barrier<> barrier_{3}
}

void oxygen() {
    mutex_.acquire();
    ++oxygen_;
    if (hydrogen_ >= 2) {
        hydro_queue_.release(2);  hydrogen_ -= 2;
        oxy_queue_.release();     --oxygen_;
    } else {
        mutex_.release();
    }
    oxy_queue_.acquire();
    hooks_.bond("O");
    barrier_.arrive_and_wait();
    mutex_.release();          // the oxygen frees the door for the next one
}
```

The completing thread holds the "mutex" across the whole bond+barrier
sequence, and the oxygen — one per molecule, whoever it is — releases it.
That is exactly why it must be a semaphore, not a `std::mutex`.

</details>
