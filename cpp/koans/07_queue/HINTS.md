# Hints — Koan 07: Queue

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

A semaphore that starts at 0 is a queue of sleepers: `acquire()` parks
you at the end of it, `release()` lets exactly one thread out. How many
queues does this ballroom need, and who releases whom?

</details>

<details>
<summary>Hint 2</summary>

Two `std::counting_semaphore<>`s, both starting at 0: one where leaders
wait, one where followers wait. Each arriving dancer releases the
*opposite* queue, then parks on its own. Announce yourself before you
wait — koan 02's lesson. No counters, no mutex: the semaphores' internal
counts do all the bookkeeping.

</details>

<details>
<summary>Hint 3</summary>

The whole solution is four lines:

```cpp
void leader_arrives() {
    follower_queue_.release();  // let one follower through…
    leader_queue_.acquire();    // …and wait until one does the same for me
}

void follower_arrives() {
    leader_queue_.release();
    follower_queue_.acquire();
}
```

with members `std::counting_semaphore<> leader_queue_{0},
follower_queue_{0};`. Swap the two lines in each method and you get the
mutual-wait deadlock the README warns about.

</details>
