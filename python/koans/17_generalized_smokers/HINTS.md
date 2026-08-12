# Hints — Koan 17: Generalized smokers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Replay koan 16's pusher with this agent: two tobacco+paper pairs land
back-to-back, so the tobacco pusher runs twice before the paper pusher
runs at all. First run: no paper noted yet, so it sets `is_tobacco =
True`. Second run: still no paper noted… `is_tobacco = True` again. That
assignment just destroyed a token — the flag was already true, and
"true" can't count to two. Two papers arrive; only one cigarette can
ever be assembled from what the scoreboard remembers. What type should
the scoreboard's entries be?

</details>

<details>
<summary>Hint 2</summary>

Keep the whole koan 16 architecture — one pusher per ingredient waiting
on its own table semaphore, one private semaphore per smoker, one mutex —
and change only the scoreboard: integer counters (`num_tobacco`,
`num_paper`, `num_match`) instead of booleans. A pusher that can't
complete a set *increments* its counter; a pusher that can, *decrements*
the counter it consumes. Increments accumulate, so bursts are remembered
instead of overwritten.

</details>

<details>
<summary>Hint 3</summary>

The tobacco pusher, in full (the other two are rotations of it):

```python
def pusher_tobacco(self):
    while True:
        self.table.tobacco.acquire()
        with self.mutex:
            if self.num_paper > 0:
                self.num_paper -= 1
                self.match_sem.release()   # wake the smoker owning match
            elif self.num_match > 0:
                self.num_match -= 1
                self.paper_sem.release()
            else:
                self.num_tobacco += 1

def smoker(self, kind, private_sem):
    while True:
        private_sem.acquire()
        self.on_smoke(kind)                # no agent_sem this time
```

The book calls this a **scoreboard**: threads file through the mutex one
at a time, read the counters like a scoreboard on the wall, and either
complete a set (take two ingredients off) or post their own. Every token
is either pending in a semaphore, counted on the scoreboard, or already
smoked — which is exactly the conservation law the tests check.

</details>
