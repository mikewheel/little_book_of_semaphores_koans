# Koan 12 — No-starve readers-writers

*Adapted from* The Little Book of Semaphores, *§4.2 (CC BY-NC-SA 4.0).*

## The problem

Koan 11's lock has a quiet cruelty in it. Nothing deadlocks — but watch a
writer under read-heavy load: readers overlap each other, so as long as a
fresh reader arrives before the last current one leaves, the room never
empties and the writer waits forever. Every individual reader behaves
reasonably; collectively they starve the writer. On a lightly loaded
system you'd never notice. Under production load, writer latency falls
off a cliff.

Fix it. Keep koan 11's safety rules exactly:

1. Readers may share the room.
2. A writer is inside alone.

…and add one fairness rule:

3. When a writer is waiting, readers already inside may finish, but
   readers who arrive **after** the writer must not pass it. Once the
   room drains, the writer goes next; the latecomers get in after it
   leaves.

## Your task

Edit `rwlock_fair.hpp`. Implement `NoStarveReadWriteLock` with the same
four methods as koan 11: `reader_enter()`, `reader_exit()`,
`writer_enter()`, `writer_exit()` — plus the fairness rule above.

## Traps worth savoring

- Koan 11's solution passes every safety test here and fails exactly one
  thing: latecomer readers stream past the queued writer. If that's the
  only test failing, you have the right baseline and the wrong doorway.
- Beware of "fixing" starvation by making readers exclusive — the
  concurrency test still demands genuinely overlapping readers when no
  writer is around.

## Modern C++ notes (many ways to skin this cat)

- `std::shared_mutex` makes **no** fairness promise; whether your writer
  starves depends on the platform. glibc's `pthread_rwlock_t` defaults to
  reader preference (writers can starve, exactly koan 11) and offers
  `PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP` to flip the policy — a
  non-portable, footgun-named escape hatch.
- The ticket-gate you build here (the book calls it a turnstile) is the
  portable answer: an outer gate every arrival passes through, which a
  waiting thread can hold shut. It converts "scheduler's whim" into
  roughly-FIFO admission, in user code, on any platform.
- Same idea, other names: two-phase admission in database lock managers,
  and the outer mutex in "fair RW lock" recipes built from
  `std::mutex` + two `condition_variable`s and generation counts.
- Note what the gate does *not* fix: strict FIFO among same-category
  threads is still up to the semaphore's wakeup order. The guarantee you
  are building is only "no latecomer reader passes a queued writer" —
  precisely what the tests check.

Run: `./check cpp 12`
