# Hints — Koan 39: Dining Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard pattern: two counters, `eating` and `ready_to_leave`, guarded
by one `mutex`, plus an `ok_to_leave` semaphore (initially 0) for the
student who has to wait.

Enumerate the states a finishing student can see. There is exactly ONE
combination where she must wait: one other student still eating, and
nobody else ready to leave alongside her.

</details>

<details>
<summary>Hint 2</summary>

Both ways out of the sticky state use "I'll do it for you": whoever
changes the situation signals `ok_to_leave` **and fixes the counters on
the waiter's behalf**, so the woken student never touches the mutex again
— she just walks out.

- A newcomer who sits down and sees `eating == 2 and ready_to_leave == 1`
  frees the waiter (and decrements `ready_to_leave` for her).
- A finisher who sees `eating == 0 and ready_to_leave == 2` frees the
  waiter and zeroes the count for both of them — they leave together.

</details>

<details>
<summary>Hint 3</summary>

```python
def student(self, sid, dine_gate=None):
    with self.mutex:
        self.eating += 1
        if self.eating == 2 and self.ready_to_leave == 1:
            self.ok_to_leave.release()      # free the stranded waiter
            self.ready_to_leave -= 1        # ...and do her bookkeeping
    self.hooks.dine(sid)
    if dine_gate is not None:
        dine_gate()
    self.mutex.acquire()
    self.eating -= 1
    self.ready_to_leave += 1
    if self.eating == 1 and self.ready_to_leave == 1:
        self.mutex.release()
        self.ok_to_leave.acquire()          # the one blocking case
    elif self.eating == 0 and self.ready_to_leave == 2:
        self.ok_to_leave.release()          # we leave together
        self.ready_to_leave -= 2
        self.mutex.release()
    else:
        self.ready_to_leave -= 1
        self.mutex.release()
    self.hooks.leave(sid)
```

with `self.mutex = threading.Lock()` (used bare on the checkout side,
since the blocking branch must release it before sleeping),
`self.ok_to_leave = threading.Semaphore(0)`, and both counters starting
at 0.

</details>
