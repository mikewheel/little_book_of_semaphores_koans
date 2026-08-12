# Hints — Koan 22: Santa Claus

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard again: `reindeer` and `elves` counters under one mutex, plus a
`santa_sem = Semaphore(0)` that Santa sleeps on. Only two arrivals ever
signal it: the reindeer that completes the herd, and the elf that
completes a group. Santa wakes, checks the counters (under the mutex!),
and decides which case he is in.

</details>

<details>
<summary>Hint 2</summary>

Waiting rooms: reindeer block on `reindeer_sem = Semaphore(0)`; Santa
releases it `n_reindeer` times after `prepare_sleigh()`. Elves block on
`elf_sem = Semaphore(0)`; Santa releases it `elf_group` times after
`help_elves()`. The missing piece is `elf_queue = Semaphore(1)` — a gate
elves pass through to register. The elf that completes a group does NOT
reopen it; the gate stays held while the group is helped, and the *last
elf to finish* `get_help` reopens it. That is what keeps elf number four
out of a batch in progress.

</details>

<details>
<summary>Hint 3</summary>

All three roles, in pseudocode:

```python
# santa daemon
while True:
    santa_sem.acquire()
    with mutex:
        if reindeer_count >= n_reindeer:
            hooks.prepare_sleigh()
            reindeer_sem.release(n_reindeer)
            reindeer_count -= n_reindeer
        elif elf_count == elf_group:
            hooks.help_elves()
            elf_sem.release(elf_group)

# reindeer_arrives(rid)
with mutex:
    reindeer_count += 1
    if reindeer_count == n_reindeer:
        santa_sem.release()
reindeer_sem.acquire()
hooks.get_hitched(rid)

# elf_needs_help(eid)
elf_queue.acquire()
with mutex:
    elf_count += 1
    if elf_count == elf_group:
        santa_sem.release()   # group full: keep the gate CLOSED
    else:
        elf_queue.release()   # room for more: hold the door open
elf_sem.acquire()
hooks.get_help(eid)
with mutex:
    elf_count -= 1
    if elf_count == 0:
        elf_queue.release()   # last one out reopens the gate
```

Checking `reindeer >= n` first is what gives reindeer priority when both
are ready. Note who resets which counter: Santa retires the herd count;
each elf checks itself out.

</details>
