# Hints — Koan 23: Building H₂O

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Use a scoreboard: counters `hydrogen` and `oxygen` protected by a mutex,
plus two queues — a semaphore hydrogens sleep on and a semaphore oxygens
sleep on. Each arrival takes the mutex, bumps its counter, and checks
whether a full molecule is now waiting (2 H and 1 O). If yes, it releases
2 tokens to the hydrogen queue and 1 to the oxygen queue — including one
for itself — and decrements the counters. If no, it releases the mutex and
goes to sleep on its queue.

</details>

<details>
<summary>Hint 2</summary>

Counters alone let molecules smear together. After `bond`, make all three
threads meet at a reusable **barrier of size 3** (you built exactly this in
koan 06 — `threading.Barrier(3)` also works). And notice: the thread that
completed the molecule *kept the mutex* — nobody released it in the if
branch. It stays held until the whole molecule has bonded, barring the
next molecule from forming. Who releases it after the barrier? Any single
one of the three works, and exactly one of them is unique per molecule:
the oxygen does it.

</details>

<details>
<summary>Hint 3</summary>

```python
# hydrogen
mutex.acquire()
hydrogen += 1
if hydrogen >= 2 and oxygen >= 1:
    hydro_queue.release(2); hydrogen -= 2
    oxy_queue.release();    oxygen -= 1
else:
    mutex.release()          # molecule incomplete: step back out
hydro_queue.acquire()
bond("H")
barrier.wait()               # all three bonded before anyone returns

# oxygen
mutex.acquire()
oxygen += 1
if hydrogen >= 2:
    hydro_queue.release(2); hydrogen -= 2
    oxy_queue.release();    oxygen -= 1
else:
    mutex.release()
oxy_queue.acquire()
bond("O")
barrier.wait()
mutex.release()              # the oxygen frees the door for the next molecule
```

The completing thread holds the mutex across the whole bond+barrier
sequence, and the oxygen — one per molecule, whoever it is — releases it.
A thread releasing a lock it didn't acquire feels illegal; for semaphores
it's just Tuesday.

</details>
