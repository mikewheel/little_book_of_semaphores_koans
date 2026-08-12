# Koan 32 — The sushi bar problem

*Adapted from* The Little Book of Semaphores, *§7.1 (CC BY-NC-SA 4.0).*

## The problem

A sushi bar has 5 seats and one social rule. If you arrive and a seat is
free — and the bar hasn't triggered the rule — you sit down immediately.
But the moment all 5 seats fill, the five of them are officially *dining
together*, and courtesy demands that nobody new squeeze in mid-party:
everyone who arrives from then on waits until the whole bar has emptied.
When the last of the party leaves, the entire waiting cohort (up to 5 of
them) takes its seats together — and if that refills the bar, the rule
re-arms for the next generation.

The rules your `SushiBar` must enforce:

1. Never more than `seats` (default 5) customers eating at once.
2. If a seat is free and the bar is **not** in must-wait mode, an
   arriving customer is seated immediately — no queueing for style
   points.
3. The bar enters must-wait mode the instant every seat is occupied, and
   stays in it until the bar is completely empty. Customers arriving
   during must-wait mode wait — *even while empty seats appear* as the
   party dwindles.
4. When the last incumbent leaves, the waiting cohort (up to `seats` of
   them) is seated together.

## Your task

Edit `sushi_bar.hpp`. Implement `SushiBar` (constructed with `seats`,
default 5) with:

- `dine(eat)` — arrive, wait if the rules demand it, then run `eat()`
  (a `const std::function<void()>&`) while seated, then leave. Callable
  from many threads at once.

## Traps worth savoring

- The obvious `counting_semaphore(5)` seats people as soon as a seat
  frees. Rule 3 says a freed seat during must-wait mode stays empty.
  There is a whole test dedicated to watching you fail this.
- The classic subtle bug (the book walks through it): a waiter who wakes
  up and then *re-locks the mutex to update the counters itself* races
  against brand-new arrivals. Newcomers can grab the mutex first, see
  stale state, and take seats — you can end up with more diners than
  seats. The fix is a change of etiquette, not more locking.

## Modern C++ notes (many ways to skin this cat)

- Reek's two correct etiquettes, translated:
  - **"I'll do it for you"** — the departing thread, already holding the
    mutex, moves the whole cohort from `waiting` to `eating` and re-arms
    `must_wait` before `block.release(n)`. Woken waiters update nothing.
    Works verbatim with `std::mutex` + `std::counting_semaphore`.
  - **"Pass the baton"** — the signaler *hands the critical section* to
    the woken thread: it releases the wait-semaphore INSTEAD of the
    mutex, and the waiter continues as if it held the lock. This is
    legal only if the "mutex" is itself a semaphore. With `std::mutex`
    it is **undefined behavior** — unlocking from a thread that didn't
    lock it violates the ownership contract (`[thread.mutex.requirements]`),
    and libstdc++/libc++ will happily corrupt or throw. POSIX says the
    same for `pthread_mutex_unlock`.
  - The idiomatic C++ translation of pass-the-baton is
    `std::condition_variable` + a predicate that encodes whose turn it
    is — ownership never migrates, only the *logical* turn does:

    ```cpp
    std::unique_lock lk(m);
    ++waiting;
    cv.wait(lk, [&] { return my_cohort_seated; });  // turn passed to us
    ```

- `notify_all` + predicate re-check also sidesteps the stale-state race
  that breaks the naive version — the re-check happens *under the lock*.
  The price is thundering-herd wakeups; `release(n)` on a semaphore is
  the surgical version.

Run: `./check cpp 32`
