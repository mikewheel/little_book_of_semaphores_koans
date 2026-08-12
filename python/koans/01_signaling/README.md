# Koan 01 — Signaling

*Adapted from* The Little Book of Semaphores, *§3.1 (CC BY-NC-SA 4.0).*

## The problem

Two threads share a semaphore. Thread A executes a statement we'll call
`a1`; thread B executes a statement we'll call `b1`. The scheduler is free
to run the threads in any interleaving — yet `b1` must never execute until
`a1` has completed.

This is **signaling**: the simplest possible use of a semaphore, enforcing a
"happens-before" edge between one event in one thread and one event in
another. It solves the serialization problem: you decide the order of two
events in two different threads without either thread spinning, polling, or
looking at a clock.

## Your task

Edit `signaling.py`. Implement:

- `Signaling.__init__` — create the semaphore(s) you need.
- `run_a(a1)` — thread A's body: call `a1()`, and arrange for B to proceed.
- `run_b(b1)` — thread B's body: call `b1()`, but only after `a1()` has
  finished. If B gets there first, it must *block* (not spin) until A is
  done.

Notes on the contract:

- A must never wait for B — signaling is one-directional.
- Works regardless of which thread starts first.

## Python notes

`threading.Semaphore(value)` is the book's semaphore: `acquire()` is the
book's `wait`, `release()` is the book's `signal`. A semaphore created with
value 0 makes any `acquire()` block until someone `release()`s — that's the
whole trick here.

Run the tests:

```sh
./check python 01
```
