# Hints — Koan 16: Cigarette smokers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Why does "smoker with match waits on tobacco, then paper" deadlock? Because
*two* smokers want tobacco as their first wait, and the semaphore doesn't
know which round it is. Whoever wakes first takes the token — even a smoker
who can never use it this round — and then blocks holding it. The broken
step is deciding *who should proceed* using only blind token-grabbing.
You need someone who can see what's on the whole table before anyone
commits to taking anything.

</details>

<details>
<summary>Hint 2</summary>

Parnas's fix: three helper threads called **pushers**, one per ingredient,
plus a scoreboard. Each pusher just waits on its own ingredient semaphore,
so tokens are never stolen across rounds. The scoreboard is three booleans
(`is_tobacco`, `is_paper`, `is_match`) under one ordinary mutex, recording
ingredients that have arrived but not yet been matched. Each smoker waits
on a *private* semaphore (`tobacco_sem`, `paper_sem`, `match_sem` — one
per owner), smokes when signaled, then releases `agent_sem`. The pushers
do all the thinking; the smokers are trivial.

</details>

<details>
<summary>Hint 3</summary>

The tobacco pusher, in full (the other two are rotations of it):

```python
def pusher_tobacco(self):
    while True:
        self.table.tobacco.acquire()
        with self.mutex:
            if self.is_paper:            # paper already on the table:
                self.is_paper = False    # tobacco+paper complete a set
                self.match_sem.release()  # wake the smoker owning match
            elif self.is_match:
                self.is_match = False
                self.paper_sem.release()
            else:
                self.is_tobacco = True   # first of the pair; note it down

def smoker(self, kind, private_sem):
    while True:
        private_sem.acquire()
        self.on_smoke(kind)
        self.table.agent_sem.release()
```

Whichever pusher runs *second* in a round sees the first pusher's note on
the scoreboard and knows the full pair — so it can name the third
ingredient's owner and wake exactly that smoker. All six threads are
daemons started in `start()`.

</details>
