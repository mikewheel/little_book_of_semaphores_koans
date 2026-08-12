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

Edit `list_guard.py`. Implement `ListGuard` with three enter/exit pairs:

- `search_enter()` / `search_exit()` — entry blocks while a deleter is
  inside; never blocks on other searchers or inserters.
- `insert_enter()` / `insert_exit()` — entry blocks while a deleter *or
  another inserter* is inside; never blocks on searchers.
- `delete_enter()` / `delete_exit()` — entry blocks until nobody at all
  is inside; while a deleter is in, everyone else blocks.

No hooks this time: the tests instrument the sections themselves.

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

Run: `./check python 27`

## Python notes

The counter-plus-first-one-locks pattern you'll likely reach for appears
in the book as a named, reusable class — worth extracting, because koan
28 wants it again. `threading.Lock` guards each counter fine; the
category-level gates want semaphores, since the thread that releases a
gate is often not the one that acquired it.
