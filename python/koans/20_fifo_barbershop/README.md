# Koan 20 — FIFO Barbershop

*Adapted from* The Little Book of Semaphores, *§5.3 (CC BY-NC-SA 4.0).*

## The problem

Koan 19's shop has a quiet injustice: up to `n` customers can be waiting on
the same semaphore, and when the barber signals it, *any* of them may win
the chair. Semaphores make no fairness promise — a customer could in
principle wait forever while later arrivals keep getting lucky.

Fix it. Same shop, same rules — capacity `n`, balk with `False` when full,
barber sleeps when idle, haircuts pair 1:1 and finish fully — plus one new
guarantee:

- **Customers are served in arrival order**, where "arrival" is the moment
  `customer_visit` registers the customer (inside its mutual exclusion).

## Your task

Edit `fifo_barbershop.py`. Implement `FifoBarbershop(n)` with the same API
as koan 19:

- `start_barber(cut_hair)` — barber daemon; now it must call waiting
  customers strictly first-come-first-served.
- `customer_visit(get_hair_cut) -> bool` — balk immediately with `False`
  when full; otherwise wait until the barber calls *you specifically*, get
  your cut, return `True` when it is fully done.

The interesting move: a single shared "your turn" semaphore cannot express
"you specifically." Something per-customer has to appear.

## Traps worth savoring

- Reusing the koan-19 solution as-is: every waiter sits on one semaphore
  and wake order is whatever the runtime feels like. **A paper-tiger
  warning**: CPython's semaphore happens to queue waiters FIFO, so the
  broken version often *passes* the order test in Python. The contract is
  still violated — no spec grants that order, and the C++ twin of this koan
  will happily serve your customers backwards. Solve it properly here too.
- Registering in the queue and signaling "a customer is ready" in the wrong
  order, or popping the queue outside the mutex — both let two threads
  disagree about who is first.

## Python notes

The pattern you are about to build — every waiter gets a private channel,
and the wake-side pops a queue and signals exactly one — is how FIFO
fairness is retrofitted onto any unfair primitive. It reappears in ticket
locks, `asyncio.Condition`'s internal waiter deque, and most RPC
dispatchers. In Python, "a semaphore per visit" is just a local
`threading.Semaphore(0)` appended to a `collections.deque`; the GC keeps it
alive as long as either side holds a reference — a luxury the C++ version
of this koan has to earn with `shared_ptr`.

Run: `./check python 20`
