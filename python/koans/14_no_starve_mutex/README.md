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
starter file — read it. It is fully adversarial in two ways: it wakes a
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

Edit `no_starve_mutex.py`. Implement `NoStarveMutex`:

- `__init__` — create your sync members. **Honor rule:** only
  `WeakSemaphore` instances and plain integers. The tests read the class
  source and reject `threading.Lock`, `threading.Semaphore`,
  `threading.Condition`, `threading.Event`, and friends inside it.
- `acquire()` — block until the lock is exclusively yours, with overtaking
  bounded.
- `release()` — hand the lock on. Called only by the current holder.

Do not modify `WeakSemaphore` itself.

## Traps worth savoring

- Wrapping a single `WeakSemaphore(1)` in `acquire`/`release` gives a
  perfectly good mutex — and fails `test_bounded_overtaking`, because
  random wakeup lets two loopers rally the token indefinitely while a
  third waits. That failure *is* the lesson; watch it happen.
- Anything that lets a fast thread loop around and rejoin the same queue
  it just left, ahead of threads that never got picked, leaves the bound
  unprovable. Morris's fix separates "threads still arriving" from
  "threads already committed" — think airlocks, not queues.

## Python notes

CPython's own `threading.Semaphore` sits on a `Condition` whose waiters
are woken in FIFO order, so in practice it behaves like a strong
semaphore — which is exactly why this koan must ship its own saboteur.
POSIX, meanwhile, guarantees you nothing: `sem_post` may wake any waiter,
and real schedulers *barge* (a running thread grabs a just-freed lock
before a woken sleeper even gets scheduled) because it's great for
throughput. Portable code that needs fairness must build it, which is
what you're about to do.

Run: `./check python 14`
