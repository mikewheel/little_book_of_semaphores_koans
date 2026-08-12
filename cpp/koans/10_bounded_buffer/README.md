# Koan 10 — Bounded buffer

*Adapted from* The Little Book of Semaphores, *§4.1 (CC BY-NC-SA 4.0).*

## The problem

Koan 09 assumed the buffer could grow forever. Real buffers can't: a disk
request queue, a network card's packet ring, an audio pipeline — all have
a fixed number of slots. That adds one constraint to the producer-consumer
rules:

- At most one thread touches the buffer at a time (it's not thread-safe).
- A consumer that finds the buffer **empty waits** for a producer.
- **New:** a producer that finds the buffer **full waits** for a consumer
  to make room. The buffer must never hold more than `capacity` items.

There's a tempting shortcut that doesn't work: "just check how many items
the counting semaphore holds before adding". Semaphores don't offer a
peek — `acquire` and `release` are the whole interface, and any value you
could read would be stale before you acted on it. The fix has a pleasing
symmetry to it.

## Your task

Edit `bounded_buffer.hpp`. Implement `BoundedBuffer<Buffer>`, constructed
with a buffer reference and an `int capacity` — same test-supplied buffer
contract as koan 09 (`add(int)`, `int get()`, not thread-safe, `get()` on
empty is a recorded error). Implement:

- `produce(int item)` — block while the buffer holds `capacity` items,
  then add `item`.
- `int consume()` — block while the buffer is empty, then remove and
  return one item.

## Traps worth savoring

- Waiting for free space *while holding exclusive buffer access* is the
  classic deadlock: the consumer that would free a slot can never reach
  the buffer. If one of the tests times out instead of failing an
  assertion, start here.
- Counting slots with an integer you check under the mutex re-invents the
  semaphore, badly — between your check and your add, the world changes.

## Modern C++ notes (many ways to skin this cat)

- This is the problem `std::counting_semaphore` is *for*. Fixed-slot rings
  are everywhere in systems code — SPSC/MPMC queues, io_uring-style
  submission/completion rings, audio callback FIFOs — and "one semaphore
  counts full slots, one counts empty slots" is the textbook shape.
- The `condition_variable` formulation needs *two* predicates ("not full"
  for producers, "not empty" for consumers) and careful notify targeting;
  the two-semaphore version encodes both counts directly and cannot lose a
  wakeup. Compare them once — it's a good taste-former.
- `std::counting_semaphore<>`'s template argument is a compile-time
  ceiling; the runtime capacity goes to the *constructor*. The default
  ceiling is fine here.
- Real-world footnote: in a hot ring buffer, producers and consumers
  hammering adjacent counters can false-share a cache line; production
  queues pad the two indices apart. Not tested here — just know the next
  cliff exists.

Run: `./check cpp 10`
