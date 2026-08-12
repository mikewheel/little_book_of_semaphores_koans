# Hints — Koan 15: Dining philosophers

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Start with the obvious representation: one binary semaphore per fork, and
`get_forks(i)` acquires the right fork then the left. Now imagine the
scheduler pauses every philosopher right after the first acquire. Five
philosophers, five forks held, everyone waiting on a neighbor — a
circular wait, the signature of deadlock. Any fix works by making that
cycle impossible. You only need to change one small thing.

(Storage detail: `std::binary_semaphore` is neither copyable nor movable,
so `std::vector<std::binary_semaphore>` won't compile. A
`std::vector<std::unique_ptr<std::binary_semaphore>>` filled in the
constructor works fine.)

</details>

<details>
<summary>Hint 2</summary>

Three classic escapes — pick ONE:

- **Footman**: a doorman semaphore initialized to `n - 1` that each
  philosopher must pass before touching forks (and signals after putting
  them down). With at most four at a five-fork table, someone can always
  get a second fork. This is koan 04's multiplex earning its keep.
- **One leftie**: make exactly one philosopher grab left-then-right while
  the rest grab right-then-left. Any deadlock cycle would need everyone
  reaching the same way around the circle — one dissenter breaks it.
- **Tanenbaum's state machine**: a per-seat state array
  (thinking/hungry/eating) and a per-philosopher semaphore, all managed
  under one mutex; a philosopher eats only when neither neighbor is
  eating, and `put_forks` re-tests both neighbors.

</details>

<details>
<summary>Hint 3</summary>

The footman version, in full:

```cpp
explicit Table(int n = 5) : n_(n), footman_(n - 1) {
    for (int i = 0; i < n; ++i)
        forks_.push_back(std::make_unique<std::binary_semaphore>(1));
}

void get_forks(int i) {
    footman_.acquire();
    forks_[right(i)]->acquire();
    forks_[left(i)]->acquire();
}

void put_forks(int i) {
    forks_[right(i)]->release();
    forks_[left(i)]->release();
    footman_.release();
}

// members:
std::counting_semaphore<> footman_;
std::vector<std::unique_ptr<std::binary_semaphore>> forks_;
```

With `n - 1` diners and `n` forks, a pigeonhole argument says some seated
philosopher has both neighbors' forks available, so progress is always
possible; and since `eat()` terminates, each fork's other claimant shows
up in bounded time — no starvation either.

For the curious: Tanenbaum's version passes every test here too, but it
is *not* starvation-free. Gingras exhibited a repeating pattern in which
two pairs of neighbors alternate meals forever while the philosopher
between them (hungry the whole time) never gets both forks. Testing for
that requires an adversarial scheduler — a nice reminder of the limits of
testing.

</details>
