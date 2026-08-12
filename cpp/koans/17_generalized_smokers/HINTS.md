# Hints — Koan 17: Generalized smokers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Replay koan 16's pusher with this agent: two tobacco+paper pairs land
back-to-back, so the tobacco pusher runs twice before the paper pusher
runs at all. First run: no paper noted yet, so it sets `have[tobacco] =
true`. Second run: still no paper noted… `have[tobacco] = true` again.
That assignment just destroyed a token — the flag was already true, and
"true" can't count to two. Two papers arrive; only one cigarette can
ever be assembled from what the scoreboard remembers. What type should
the scoreboard's entries be?

</details>

<details>
<summary>Hint 2</summary>

Keep the whole koan 16 architecture — one pusher per ingredient waiting
on its own table semaphore, one private semaphore per smoker, one mutex —
and change only the scoreboard: integer counters instead of booleans. A
pusher that can't complete a set *increments* its counter; a pusher that
can, *decrements* the counter it consumes. Increments accumulate, so
bursts are remembered instead of overwritten. (And no `agent_sem`
release in the smoker this time — nobody is listening.)

</details>

<details>
<summary>Hint 3</summary>

With ingredients as indices (`0=tobacco, 1=paper, 2=match`), all three
pushers are one function — for pusher `k`, the other two ingredients are
`a = (k+1) % 3` and `b = (k+2) % 3`:

```cpp
static void pusher(std::shared_ptr<State> st, int k) {
    int a = (k + 1) % 3, b = (k + 2) % 3;
    for (;;) {
        st->table->ingredient_sem(k).acquire();
        std::lock_guard lock(st->mutex);
        if (st->counts[a] > 0) {      // {k, a} complete a set:
            --st->counts[a];          // wake the owner of b
            st->smoker_sem(b).release();
        } else if (st->counts[b] > 0) {
            --st->counts[b];
            st->smoker_sem(a).release();
        } else {
            ++st->counts[k];          // note it down — every single time
        }
    }
}

static void smoker(std::shared_ptr<State> st, int k) {
    for (;;) {
        st->smoker_sem(k).acquire();
        st->on_smoke(kIngredients[k]);   // no agent_sem this time
    }
}
```

The book calls this a **scoreboard**: threads file through the mutex one
at a time, read the counters like a scoreboard on the wall, and either
complete a set (take two ingredients off) or post their own. Every token
is either pending in a semaphore, counted on the scoreboard, or already
smoked — which is exactly the conservation law the tests check.

</details>
