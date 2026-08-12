# Hints — Koan 31: The Modus Hall problem

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

This is a **scoreboard** problem. Keep counts of *both* factions'
checked-in members, plus a status field with five values: `neutral`,
`heathens rule`, `prudes rule`, `transition to heathens`, `transition to
prudes`. Every arrival and every departure is a case analysis on that
status, under one mutex. Decide what each case does before you touch a
semaphore.

</details>

<details>
<summary>Hint 2</summary>

A roster that works:

- `heathens`, `prudes` — checked-in counts (waiting *and* crossing).
- `status` — the five-state field from Hint 1.
- `mutex` — guards all of the above.
- `heathen_turn`, `prude_turn` = `Semaphore(1)` — one turnstile per
  faction, at the door. Locking `prude_turn` bars new prudes during a
  transition to heathen control, and vice versa.
- `heathen_queue`, `prude_queue` = `Semaphore(0)` — where checked-in
  members wait for their faction's turn; released as a whole batch.

Arrivals pass their own turnstile (acquire, then immediately release),
check in under the mutex, then either cross or wait on their queue. The
arrival that tips the majority sets `status` to "transition" and locks
the *opponents'* turnstile. Departures decrement, and the *last* one out
hands the path to the waiting cohort — releasing the queue `n` times —
and reopens whichever turnstile the transition had locked.

</details>

<details>
<summary>Hint 3</summary>

Heathen check-in and check-out (prudes are the mirror image):

```python
heathen_turn.acquire(); heathen_turn.release()   # the door

with-mutex:
    heathens += 1
    if status == 'neutral':          status = 'heathens rule'; go
    elif status == 'prudes rule':
        if heathens > prudes:        # we just tipped it
            status = 'transition to heathens'
            prude_turn.acquire()     # bar new prudes at their door
        wait on heathen_queue        # (release mutex first)
    elif status == 'transition to heathens':  wait on heathen_queue
    else:                            go   # heathens rule / transition to prudes

# cross()

with-mutex:
    heathens -= 1
    if heathens == 0:                # last one out flips the field
        if status == 'transition to prudes':
            heathen_turn.release()   # reopen the door the transition locked
        if prudes: prude_queue.release(prudes); status = 'prudes rule'
        else:      status = 'neutral'
    elif status == 'heathens rule' and prudes > heathens:
        status = 'transition to prudes'   # our departure tipped it
        heathen_turn.acquire()
```

Note the book's own caveat: threads that have passed the turnstile but
not yet checked in aren't counted — the majority is over *registered
voters* only, and the tests honor that definition. (If you compare this
with the book's printed solution: the printed last-one-out branch signals
the wrong turnstile — reopen the one the transition actually locked, as
above, or a faction can be locked out forever.)

</details>
