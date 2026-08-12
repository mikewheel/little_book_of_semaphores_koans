# Koan 22 — Santa Claus

*Adapted from* The Little Book of Semaphores, *§5.5 (CC BY-NC-SA 4.0).*

## The problem

John Trono's North Pole puzzle (via William Stallings). Santa sleeps.
Exactly two things may wake him:

- **All the reindeer are home.** The herd trickles back from vacation one
  by one; the last arrival rouses Santa. He preps the sleigh once, then
  every reindeer gets hitched, and off they go.
- **A group of elves is stuck.** Elves only dare disturb Santa in groups
  of `elf_group` (three, canonically). The group's last member wakes him;
  Santa helps the whole group at once.

The synchronization constraints:

1. Santa does nothing until a full herd (`n_reindeer`) or a full group
   (`elf_group`) is ready — partial groups keep waiting.
2. A complete herd → `prepare_sleigh()` exactly once, then all
   `n_reindeer` reindeer run `get_hitched(rid)`.
3. A complete elf group → `help_elves()` exactly once, then exactly the
   group's members run `get_help(eid)` — no more, no fewer.
4. While a group is being helped, later elves cannot start forming the
   next group; they wait until the current group is fully done.
5. Santa loops forever: many flights, many elf groups.

The book also gives reindeer priority over elves when both are ready at
once. Our tests leave that property alone — it is real but brutally
timing-dependent to observe — so treat it as a bonus constraint to reason
about (the shape of Santa's wake-up check decides it).

## Your task

Edit `santa.hpp`. The tests construct `NorthPole(hooks, n_reindeer = 9,
elf_group = 3)`; the provided `NorthPoleHooks` struct carries
`prepare_sleigh()`, `get_hitched(rid)`, `help_elves()`, `get_help(eid)`.
Implement:

- `start_santa()` — Santa as a detached daemon thread, looping forever.
- `reindeer_arrives(rid)` — blocks until this year's sleigh is prepped,
  then calls `hooks_.get_hitched(rid)` and returns.
- `elf_needs_help(eid)` — blocks until Santa has helped this elf's group,
  then calls `hooks_.get_help(eid)` and returns. The `get_help` calls of a
  group happen after that group's `help_elves()`, and a late elf must
  land in a *later* group.

One assumption you may lean on, faithful to the story: it is the *same*
`n_reindeer` reindeer every year, so a new herd never starts arriving
until the previous flight is fully hitched. Elves come in unbounded
numbers at any time — your group gate is what tames them.

## Traps worth savoring

- Waking Santa is easy; making him wake *once per complete group* is the
  puzzle. If every arrival signals him, Santa leaps up for partial groups
  and the exactly-once counts drift.
- The classic subtle bug is at the elf boundary: without a gate that the
  *third* elf holds closed (and the *last leaver* reopens), a fourth elf
  slips into a group that is already being helped — constraint 4 dies.
- Letting Santa decrement the herd/group counters on the workers' behalf
  works, but be precise about who owns which counter when; the book's
  solution splits it (Santa resets the reindeer count, elves decrement
  their own) and the tests will notice double-counting either way.

## Modern C++ notes (many ways to skin this cat)

- This is the **batching** pattern — accumulate under a mutex, and one
  thread flips the whole batch at once. You will meet it again in GC
  safepoints, database group commit, and every "flush when the buffer is
  full" design.
- Releasing the herd is one call: `reindeer_sem.release(9)`. A
  `counting_semaphore::release(n)` is not just sugar for a signal loop —
  it can wake all `n` waiters with one notify under the hood. But mind the
  template ceiling: `counting_semaphore<9>` means *at most* 9 tokens ever;
  release past the ceiling is UB, not an exception. (`release(n)` on a
  `binary_semaphore` with `n > 1` is the same trap in miniature.)
- Why is Santa his own thread at all? The ninth reindeer could do the work
  itself — the book calls this pattern "I'll do it for you," and it saves
  a context switch. The dedicated thread wins on *ownership clarity*:
  exactly one thread reads the scoreboard and resets it, and the
  arrivals' critical sections stay tiny. When you find yourself unsure
  who is allowed to touch a counter, that's the design telling you to
  hire a Santa.

Run: `./check cpp 22`
