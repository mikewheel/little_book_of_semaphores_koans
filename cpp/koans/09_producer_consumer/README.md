# Koan 09 — Producer-consumer

*Adapted from* The Little Book of Semaphores, *§4.1 (CC BY-NC-SA 4.0).*

## The problem

A classic division of labor: some threads *make* things, others *use* them.
Think of an event loop — input handlers, network readers, and timers all
push event objects into a shared buffer, while handler threads pull them
out and process them. The buffer is the meeting point, and it is fragile:

- The buffer is **not thread-safe**. While any thread is midway through an
  `add` or a `get`, the buffer is in an inconsistent state, so at most one
  thread may be touching it at any instant.
- A consumer that finds the buffer **empty must wait** until a producer
  delivers something.
- The buffer is unbounded in this koan, so a producer never has a reason
  to wait for long — `produce` must not block indefinitely.

Note what is *not* required: nothing says the items come out in the order
they went in. Any item will do; the buffer is a bag, not a queue.

## Your task

Edit `event_buffer.hpp`. Implement `ProducerConsumer<Buffer>`, constructed
with a reference to a test-supplied buffer exposing exactly two methods:
`buffer.add(int item)` and `int buffer.get()`. The tests instrument it to
detect concurrent access, and calling `get()` on an empty buffer is an
error it records. Implement:

- `produce(int item)` — hand `item` to the buffer. May contend for
  exclusive access, but must never block waiting for a consumer.
- `int consume()` — block while the buffer is empty, then remove and
  return one item.

## Traps worth savoring

- Announcing an item *before* it is actually in the buffer opens a window
  where a consumer wakes up, wins the race to the buffer, and `get`s from
  a buffer that is still empty. The instrumented buffer notices.
- Waiting for an item *while holding exclusive access to the buffer* is
  the deadly version: the producer that would wake you can never get in.
  That one is a deadlock, and the book devotes a whole section to it.

## Modern C++ notes (many ways to skin this cat)

- In production this is `std::queue` + `std::mutex` +
  `std::condition_variable` (wait on a "not empty" predicate), or a
  ready-made MPMC queue (TBB, moodycamel, folly) when throughput matters.
  The semaphore formulation is worth learning anyway: a
  `std::counting_semaphore` whose *value is the item count* collapses the
  predicate-and-wait dance into two lines and cannot suffer a lost wakeup.
- The book writes `mutex.wait()` / `mutex.signal()` explicitly; idiomatic
  C++ prefers a scoped `std::lock_guard`/`std::scoped_lock`, which cannot
  forget to release on an early return or an exception.
- Where you signal matters for performance: releasing the item-count
  semaphore while still holding the mutex can wake a consumer straight
  into a lock it cannot take yet. Signal after the critical section.
- Lock-free MPMC queues exist precisely because this lock is a contention
  point — but get the locked version right first; the lock-free ones are
  graduate-level UB bait.

Run: `./check cpp 09`
