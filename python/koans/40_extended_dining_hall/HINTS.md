# Hints — Koan 40: Extended Dining Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Mirror koan 39's leave-side machinery on the eat side: a third counter
`ready_to_eat`, and a second parking semaphore `ok_to_sit` (initially 0)
for the student waiting with her tray.

The one eat-side blocking state: `eating == 0 and ready_to_eat == 1`
(that 1 is you). The only escape: a second ready-to-eat student arrives.

</details>

<details>
<summary>Hint 2</summary>

The symmetric case analysis after `get_food` (all under the mutex):

- `eating == 0 and ready_to_eat == 1` → release the mutex and wait on
  `ok_to_sit`; your rescuer does your bookkeeping ("I'll do it for you").
- `eating == 0 and ready_to_eat == 2` → signal `ok_to_sit`, move *both*
  of you from ready-to-eat to eating, and proceed.
- otherwise (someone is dining) → seat yourself immediately — and this
  is the branch that inherits koan 39's newcomer duty: if you now see
  `eating == 2 and ready_to_leave == 1`, free the stranded leaver.

The checkout is exactly koan 39's three cases, unchanged. Note the happy
fact from the book's analysis: a pair sitting down at an empty table
never needs to check for stranded leavers — nobody can be mid-meal at an
empty table.

</details>

<details>
<summary>Hint 3</summary>

```python
def student(self, sid, dine_gate=None):
    self.hooks.get_food(sid)

    self.mutex.acquire()
    self.ready_to_eat += 1
    if self.eating == 0 and self.ready_to_eat == 1:
        self.mutex.release()
        self.ok_to_sit.acquire()            # wait for company
    elif self.eating == 0 and self.ready_to_eat == 2:
        self.ok_to_sit.release()            # we sit down together
        self.ready_to_eat -= 2
        self.eating += 2
        self.mutex.release()
    else:
        self.ready_to_eat -= 1
        self.eating += 1
        if self.eating == 2 and self.ready_to_leave == 1:
            self.ok_to_leave.release()      # koan 39's newcomer duty
            self.ready_to_leave -= 1
        self.mutex.release()

    self.hooks.dine(sid)
    if dine_gate is not None:
        dine_gate()

    self.mutex.acquire()
    self.eating -= 1
    self.ready_to_leave += 1
    if self.eating == 1 and self.ready_to_leave == 1:
        self.mutex.release()
        self.ok_to_leave.acquire()
    elif self.eating == 0 and self.ready_to_leave == 2:
        self.ok_to_leave.release()
        self.ready_to_leave -= 2
        self.mutex.release()
    else:
        self.ready_to_leave -= 1
        self.mutex.release()

    self.hooks.leave(sid)
```

Members: `mutex = threading.Lock()`, `ok_to_sit = threading.Semaphore(0)`,
`ok_to_leave = threading.Semaphore(0)`, and the three counters at 0.

</details>
