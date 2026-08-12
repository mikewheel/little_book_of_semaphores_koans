# Hints — Koan 24: River crossing

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Scoreboard again: counters `hackers` and `serfs` under one mutex, plus two
queues (a semaphore hackers sleep on, one serfs sleep on). Each arrival
bumps its counter and asks: did I just complete a crew? Three completing
conditions exist — 4 hackers, 4 serfs, or 2+2. The completer releases the
right mix of queue tokens (including one for itself), zeroes/decrements
the counters, and remembers *in a local variable* that it is the captain.

</details>

<details>
<summary>Hint 2</summary>

Two more pieces:

- The completer does **not** release the mutex — it keeps the dock closed
  so no fifth passenger can slip into this boatload. It releases the mutex
  only after the boat has sailed.
- All four crew members meet at a reusable **barrier of size 4** after
  boarding (koan 06). Once the barrier opens, everyone knows all four
  boards happened; the captain — flagged by that local `is_captain` — rows
  and then reopens the dock.

</details>

<details>
<summary>Hint 3</summary>

```python
def _arrive(self, kind):
    is_captain = False              # local: private to this thread
    self.mutex.acquire()
    bump counter for kind
    if self.hackers == 4:
        self.hacker_queue.release(4); self.hackers = 0; is_captain = True
    elif self.serfs == 4:
        self.serf_queue.release(4); self.serfs = 0; is_captain = True
    elif self.hackers >= 2 and self.serfs >= 2:
        self.hacker_queue.release(2); self.serf_queue.release(2)
        self.hackers -= 2; self.serfs -= 2; is_captain = True
    else:
        self.mutex.release()        # incomplete crew: step back out

    (own queue).acquire()
    self.hooks.board(kind)
    self.barrier.wait()             # Barrier(4): all aboard before rowing
    if is_captain:
        self.hooks.row_boat(kind)
        self.mutex.release()        # boat has sailed: reopen the dock
```

`hacker_arrives` / `serf_arrives` both delegate here. The captain holds
the mutex from crew completion until after `row_boat` — that hold is what
keeps boatloads from interleaving.

</details>
