# Koan 03 — Mutex

*Adapted from* The Little Book of Semaphores, *§3.4 (CC BY-NC-SA 4.0).*

## The problem

Two (or two hundred) threads each execute `count = count + 1` on a shared
variable. An increment is secretly a read followed by a write, and the
scheduler may interleave the threads between those steps — so updates get
lost. The classic fix is **mutual exclusion**: wrap the update in a
*critical section* that at most one thread can occupy at a time.

Build that guard out of a semaphore. A semaphore used this way is a token
passed between threads: to enter the critical section you must hold the
token; leaving hands it back.

## Your task

Edit `mutex.py`. Implement a `Mutex` class backed by a semaphore:

- `__init__` — what initial value means "one thread may enter"?
- `acquire()` — block until the critical section is free, then claim it.
- `release()` — leave the critical section, admitting one waiter (if any).

The solution is symmetric — every thread runs the same two calls — and it
must work for *any* number of threads, not just two.

## Python notes

Yes, CPython has a GIL, and no, it does not save you: the tests split each
increment into an explicit read and write with a scheduling gap between
them, which is exactly what the interpreter can do to *any* `x = x + 1` at
bytecode level. Lost updates happen in pure Python.

(`threading.Lock` is the everyday tool for this; the koan asks you to make
one from a `threading.Semaphore` to see that a lock *is* a semaphore worn
differently.)

Run: `./check python 03`
