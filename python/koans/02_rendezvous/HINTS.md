# Hints — Koan 02: Rendezvous

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

This is two signaling patterns (koan 01) pointed at each other. How many
semaphores does that suggest, and what initial values?

</details>

<details>
<summary>Hint 2</summary>

Two semaphores, both starting at 0: `a_arrived` and `b_arrived`. Each
thread announces its own arrival and waits for the other's. The question
that separates the working answers from the deadlock: in each thread, do
you *signal first* or *wait first*?

</details>

<details>
<summary>Hint 3</summary>

Signal, then wait:

- A: `a1()` → `a_arrived.release()` → `b_arrived.acquire()` → `a2()`
- B: `b1()` → `b_arrived.release()` → `a_arrived.acquire()` → `b2()`

If both threads *wait first*, each blocks before it can announce itself:
deadlock. (Having exactly one thread wait first also works, at the cost of
an extra context switch — see the book's discussion.)

</details>
