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
under one `std::mutex`, recording ingredients that have arrived but not
yet been matched. Each smoker waits on a *private*
`std::counting_semaphore` (one per owner), smokes when signaled, then
releases `agent_sem`. The pushers do all the thinking; the smokers are
trivial.

Structure: one heap-allocated `State` (booleans, mutex, three private
semaphores, the `shared_ptr<AgentTable>`, the `on_smoke` callback) held
by `shared_ptr`; six detached threads each capture it by value.

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
        if (st->have[a]) {          // {k, a} complete a set:
            st->have[a] = false;    // wake the owner of b
            st->smoker_sem(b).release();
        } else if (st->have[b]) {
            st->have[b] = false;
            st->smoker_sem(a).release();
        } else {
            st->have[k] = true;     // first of the pair; note it down
        }
    }
}

static void smoker(std::shared_ptr<State> st, int k) {
    for (;;) {
        st->smoker_sem(k).acquire();
        st->on_smoke(kIngredients[k]);
        st->table->agent_sem.release();
    }
}
```

`start()` spawns `pusher` and `smoker` for `k = 0, 1, 2`, all detached.
Whichever pusher runs *second* in a round sees the first pusher's note on
the scoreboard and knows the full pair — so it can name the third
ingredient's owner and wake exactly that smoker.

</details>
