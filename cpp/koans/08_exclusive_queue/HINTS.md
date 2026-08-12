# Hints — Koan 08: Exclusive queue

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 07 got away without state because nobody needed to *ask* anything —
but here an arriving dancer must know "is a partner already waiting?",
and a semaphore cannot be asked its value. So keep a scoreboard: two
counters (waiting leaders, waiting followers) guarded by a mutex. An
arriving dancer either claims a waiting partner (decrement their
counter, release their queue) or increments its own counter and parks on
its own queue.

</details>

<details>
<summary>Hint 2</summary>

The exclusivity comes from an **asymmetric hand-off of the guard**: the
dancer who completes the pair does *not* release it — the pair keeps the
guard for the whole dance, so no other pair can even reach the
scoreboard. The leader releases it at the very end, after a final
rendezvous semaphore tells it the partner's dance has finished. Note
what that implies: the guard is acquired by one thread and released by a
different one — so it **cannot be a `std::mutex`** (undefined behavior);
use a `std::binary_semaphore{1}`. (And a dancer who parks must release
the guard *before* sleeping on its queue, or the ballroom deadlocks —
koan 05's lesson.)

</details>

<details>
<summary>Hint 3</summary>

Book-style solution, both sides:

```cpp
void leader_dances(const std::function<void()>& dance) {
    mutex_.acquire();
    if (followers_ > 0) {
        --followers_;
        follower_queue_.release();  // claim a waiting partner
    } else {
        ++leaders_;
        mutex_.release();           // let go before parking!
        leader_queue_.acquire();    // woken by a follower who kept it
    }
    dance();
    rendezvous_.acquire();          // partner has finished dancing
    mutex_.release();               // the pair's guard, whoever took it
}

void follower_dances(const std::function<void()>& dance) {
    mutex_.acquire();
    if (leaders_ > 0) {
        --leaders_;
        leader_queue_.release();
    } else {
        ++followers_;
        mutex_.release();
        follower_queue_.acquire();
    }
    dance();
    rendezvous_.release();          // tell the leader we're done
}
```

with members `int leaders_ = 0, followers_ = 0;
std::binary_semaphore mutex_{1};
std::counting_semaphore<> leader_queue_{0}, follower_queue_{0},
rendezvous_{0};`.

Exactly one dancer per pair takes the guard and keeps it through the
dance; the leader always gives it back. That is why only one pair can be
on the floor, and why the leader cannot leave early.

</details>
