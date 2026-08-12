# Koan 14 — No-starve mutex

*Adapted from* The Little Book of Semaphores, *§4.3 (CC BY-NC-SA 4.0).*

## The problem

Every mutex you've built so far had a silent benefactor: a well-behaved
semaphore. When several threads wait and someone signals, *which* waiter
wakes up? A **strong** semaphore promises the answer is fair — nobody sits
in the queue while a bounded number of others jump past. A **weak**
semaphore promises only that *somebody* wakes up.

With a weak semaphore, the ordinary one-semaphore mutex can starve a
thread. Picture threads A, B, and C looping on the same lock: A holds it,
B and C wait. A releases, B wins; before B releases, A is back in the
queue. B releases, and the semaphore — free to pick anyone — picks A
again. B rejoins. This tennis match between A and B can rally forever
while C grows old in the queue.

Dijkstra conjectured that starvation-free mutual exclusion was impossible
with weak semaphores alone. In 1979 J.M. Morris proved him wrong. This
koan is Morris's construction.

Your building block is the `WeakSemaphore` provided at the top of the
starter header — read it. It is fully adversarial in two ways: it wakes a
*random* waiter, and a token released when nobody is queued can be
snatched by a thread that arrives *later* than one still mid-approach.

The guarantee you must provide, assuming a finite number of threads:

- **Mutual exclusion** — at most one thread between `acquire()` and
  `release()`.
- **Bounded overtaking** — once a thread calls `acquire()`, the number of
  times *other* threads can be granted the lock before it gets in is
  bounded (the tests allow roughly two rooms full of peers, not one grant
  more).
- **Progress** — no deadlock, ever.

## Your task

Edit `no_starve_mutex.hpp`. Implement `NoStarveMutex`:

- add your members — **honor rule:** only `WeakSemaphore` instances and
  plain integers; no `std::mutex`, `std::counting_semaphore`, or
  `std::condition_variable` of your own;
- `acquire()` — block until the lock is exclusively yours, with overtaking
  bounded;
- `release()` — hand the lock on. Called only by the current holder.

Do not modify `WeakSemaphore` itself.

## Traps worth savoring

- Wrapping a single `WeakSemaphore{1}` in `acquire`/`release` gives a
  perfectly good mutex — and fails the `bounded_overtaking` test, because
  random wakeup lets two loopers rally the token indefinitely while a
  third waits. That failure *is* the lesson; watch it happen.
- Anything that lets a fast thread loop around and rejoin the same queue
  it just left, ahead of threads that never got picked, leaves the bound
  unprovable. Morris's fix separates "threads still arriving" from
  "threads already committed" — think airlocks, not queues.

## Modern C++ notes (many ways to skin this cat)

- `std::mutex` makes **no fairness promise** either. Real implementations
  sit on futexes that deliberately let a running thread *barge* past woken
  sleepers, because handing the lock strictly in FIFO order costs a
  context switch per handoff. Unfair-by-default is an engineering choice,
  not an accident — which is why "just use the OS mutex" does not dissolve
  this koan.
- The modern industrial answers to fairness are **ticket locks** (take a
  number, spin until the now-serving counter reaches it) and **MCS/CLH
  queue locks** (each waiter spins on its own cache line — this is roughly
  what OS kernels and `std::mutex` slow paths use). Morris's algorithm is
  their semaphore-only ancestor: same idea of an explicit arrival order,
  built with 1979 parts.
- Spinning vs blocking: a ticket lock that spins with
  `std::this_thread::yield()` burns CPU but gets perfect FIFO for free; a
  blocking semaphore sleeps cheaply but surrenders wakeup order to the
  implementation. Morris shows you can recover the bound *without*
  spinning.
- The provided `WeakSemaphore` is itself a nice study in
  condition-variable hygiene: one `std::condition_variable` per waiter, a
  `granted` flag to survive spurious wakeups, and `shared_ptr` ownership
  so a waiter's ticket cannot dangle while `release()` still holds it.

Run: `./check cpp 14`
