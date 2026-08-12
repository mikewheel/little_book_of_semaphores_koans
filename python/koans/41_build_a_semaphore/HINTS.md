# Hints — Koan 41: Build a semaphore

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

The book's ingredient list, translated to Python: an integer `value`, a
second integer `wakeups`, and one `threading.Condition` (which carries
its own lock — use it for both counters).

`value` may go negative: a negative value counts how many threads are
waiting. `wakeups` counts signals that have been sent but not yet
consumed by a woken thread.

</details>

<details>
<summary>Hint 2</summary>

Why `wakeups` exists: with only `value` and a `while value <= 0:
cond.wait()` loop, a releaser can signal and then *race back around* and
call `acquire()` before the sleeper resumes — the fresh caller sees a
positive value and takes it, and the sleeper wakes to find nothing. The
signal was meant for a *waiter* (the book's Property 3), so the woken
thread must consume a `wakeups` token that fresh arrivals never touch.

The book writes this as a do-while: sleep *first*, then check. Python
has no do-while, so the shape becomes:

```python
while True:
    cond.wait()
    if wakeups >= 1:
        break
wakeups -= 1
```

The re-loop also absorbs spurious wakeups, which is why the standard
idiom for condition variables is always a loop, never a bare `if`.

</details>

<details>
<summary>Hint 3</summary>

```python
def __init__(self, value=0):
    if value < 0:
        raise ValueError("initial semaphore value must be nonnegative")
    self.value = value
    self.wakeups = 0
    self.cond = threading.Condition()

def acquire(self):
    with self.cond:
        self.value -= 1
        if self.value < 0:
            while True:
                self.cond.wait()
                if self.wakeups >= 1:
                    break
            self.wakeups -= 1

def release(self):
    with self.cond:
        self.value += 1
        if self.value <= 0:          # someone is waiting
            self.wakeups += 1
            self.cond.notify()
```

Note that `release()` only notifies when the value was negative — a
banked permit with nobody waiting needs no wakeup, which is exactly
Property 2 falling out of the arithmetic.

</details>
