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

Edit `event_buffer.py`. Implement `ProducerConsumer(buffer)`, where
`buffer` is a test-supplied object exposing exactly two methods:
`buffer.add(item)` and `buffer.get() -> item`. The tests instrument it to
detect concurrent access, and calling `get()` on an empty buffer is an
error it records. Implement:

- `produce(item)` — hand `item` to the buffer. May contend for exclusive
  access, but must never block waiting for a consumer.
- `consume() -> item` — block while the buffer is empty, then remove and
  return one item.

## Traps worth savoring

- Announcing an item *before* it is actually in the buffer opens a window
  where a consumer wakes up, wins the race to the buffer, and `get`s from
  a buffer that is still empty. The instrumented buffer notices.
- Waiting for an item *while holding exclusive access to the buffer* is
  the deadly version: the producer that would wake you can never get in.
  That one is a deadlock, and the book devotes a whole section to it.

## Python notes

`queue.Queue` is this entire koan in the stdlib — `put`/`get` with all the
locking and waiting built in. Here you build the machinery yourself, which
is exactly what `queue.Queue` does under the hood (a lock plus not-empty
bookkeeping). Knowing the innards pays off the day you need semantics the
stdlib doesn't stock.

Run: `./check python 09`
