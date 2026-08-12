# Hints — Koan 33: Child care

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Think of supervision as tokens: every adult on the floor contributes
`ratio` child-permits. A child entering consumes one permit; a child
leaving returns one. An arriving adult mints `ratio` fresh permits — and a
departing adult must take `ratio` permits back out of circulation before
walking off. One counting semaphore *almost* carries the whole design.

</details>

<details>
<summary>Hint 2</summary>

The almost: a leaving adult does `ratio` separate `acquire()` calls. Two
adults leaving concurrently can split the available permits between them —
each holds some, needs more, and neither can back out. That is the deadlock
the tests provoke. The minimal fix: make the *batch* of acquires atomic by
wrapping them in a mutex that only leavers touch. Then whoever holds the
mutex drains its full batch (blocking inside if permits are short), and the
next leaver queues behind it.

Waiting on a semaphore while holding a lock is normally a red flag — here
it is safe because only leavers ever take that lock, and the permits they
wait for come from `child_leave`/`adult_enter`, which never touch it.

</details>

<details>
<summary>Hint 3</summary>

```text
permits = Semaphore(0)      # child-permits in circulation
leave_lock = Mutex()        # serializes departing adults

adult_enter:  permits.release(ratio)
child_enter:  permits.acquire()
child_leave:  permits.release()
adult_leave:  with leave_lock:
                  repeat ratio times: permits.acquire()
```

The invariant holds because a child is only inside while holding a permit,
and permits in circulation never exceed `ratio ×` (adults present).

</details>
