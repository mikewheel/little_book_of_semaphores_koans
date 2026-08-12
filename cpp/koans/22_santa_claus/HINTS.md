# Hints — Koan 22: Santa Claus

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard again: `reindeer_` and `elves_` counters under one mutex, plus
a `santa_sem_{0}` that Santa sleeps on. Only two arrivals ever signal it:
the reindeer that completes the herd, and the elf that completes a group.
Santa wakes, checks the counters (under the mutex!), and decides which
case he is in.

</details>

<details>
<summary>Hint 2</summary>

Waiting rooms: reindeer block on `reindeer_sem_{0}`; Santa releases it
`n_reindeer` times (one `release(n_reindeer_)` call) after
`prepare_sleigh()`. Elves block on `elf_sem_{0}`; Santa releases it
`elf_group` times after `help_elves()`. The missing piece is
`elf_queue_{1}` — a gate elves pass through to register. The elf that
completes a group does NOT reopen it; the gate stays held while the group
is helped, and the *last elf to finish* `get_help` reopens it. That is
what keeps elf number four out of a batch in progress.

</details>

<details>
<summary>Hint 3</summary>

All three roles, in pseudocode:

```cpp
// santa daemon
while (true) {
    santa_sem_.acquire();
    std::lock_guard lock(mutex_);
    if (reindeer_ >= n_reindeer_) {
        hooks_.prepare_sleigh();
        reindeer_sem_.release(n_reindeer_);
        reindeer_ -= n_reindeer_;
    } else if (elves_ == elf_group_) {
        hooks_.help_elves();
        elf_sem_.release(elf_group_);
    }
}

// reindeer_arrives(rid)
{
    std::lock_guard lock(mutex_);
    if (++reindeer_ == n_reindeer_) santa_sem_.release();
}
reindeer_sem_.acquire();
hooks_.get_hitched(rid);

// elf_needs_help(eid)
elf_queue_.acquire();
{
    std::lock_guard lock(mutex_);
    if (++elves_ == elf_group_)
        santa_sem_.release();     // group full: keep the gate CLOSED
    else
        elf_queue_.release();     // room for more: hold the door open
}
elf_sem_.acquire();
hooks_.get_help(eid);
{
    std::lock_guard lock(mutex_);
    if (--elves_ == 0) elf_queue_.release();  // last one out reopens
}
```

Checking `reindeer_ >= n_reindeer_` first is what gives reindeer priority
when both are ready. Note who resets which counter: Santa retires the
herd count; each elf checks itself out.

</details>
