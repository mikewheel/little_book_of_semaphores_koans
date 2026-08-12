# Koan 27 — Search-Insert-Delete

*Adapted from* The Little Book of Semaphores, *§6.1 (CC BY-NC-SA 4.0).*

## The problem

Three kinds of threads work on one shared singly-linked list, and each
kind tolerates different company:

- **Searchers** only read. Any number may run concurrently with each
  other — and with an inserter.
- **Inserters** append to the end of the list. Two inserters at once
  would race on the tail pointer, so inserters exclude *each other* — but
  a single inserter coexists happily with any number of searchers.
- **Deleters** unlink nodes from anywhere in the list. A deleter must be
  completely alone: no searchers, no inserters, no other deleters.

This is *categorical* mutual exclusion — the readers-writers problem with
a third category wedged between "read" and "write".

## Your task

Edit `list_guard.hpp`. Implement `ListGuard` with three enter/exit pairs:

- `search_enter()` / `search_exit()` — entry blocks while a deleter is
  inside; never blocks on other searchers or inserters.
- `insert_enter()` / `insert_exit()` — entry blocks while a deleter *or
  another inserter* is inside; never blocks on searchers.
- `delete_enter()` / `delete_exit()` — entry blocks until nobody at all
  is inside; while a deleter is in, everyone else blocks.

No hooks this time: the tests instrument the sections themselves.

Run: `./check cpp 27`

## Traps worth savoring

- One big mutex "solves" every constraint except the ones about
  concurrency: the tests demand that four searchers actually share, and
  that an inserter works *while* searchers are inside. Over-locking fails
  here just as surely as under-locking.
- Guarding the inserter against deleters but forgetting the
  inserter-vs-inserter mutex (or vice versa) — the two exclusions are
  independent and both required.
- A deleter that grabs its permissions in one order while another deleter
  grabs them in the other order is a textbook deadlock recipe. Pick one
  global order.

## Modern C++ notes (many ways to skin this cat)

- Squint and this is a three-way generalization of `std::shared_mutex`:
  searchers are `lock_shared()`, deleters are `lock()`, and inserters are
  a category the standard type simply cannot express — shared with
  readers, exclusive among themselves. When your access taxonomy outgrows
  reader/writer, you're back to composing semaphores (or a
  `condition_variable` with a scoreboard of three counters).
- The industrial-strength answer for read-mostly linked structures is
  **RCU** (read-copy-update, the Linux kernel's workhorse) or a seqlock.
  The RCU mapping is beautifully close to this koan: searchers are
  wait-free readers who never block; inserters publish with a single
  atomic pointer store; and a deleter's "wait until nobody is inside" is
  literally RCU's *grace period* — unlink now, wait for all pre-existing
  readers to drain, then reclaim. This koan is RCU with the grace period
  made painfully explicit.
- Enter/exit pairs scream RAII: one exception between `search_enter` and
  `search_exit` and the whole system wedges. In production you'd wrap
  each pair in a guard object (see koan 28's notes for a sketch).
