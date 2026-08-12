# Hints — Koan 27: Search-Insert-Delete

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Think in terms of two "coast is clear" flags, each a
`std::binary_semaphore{1}`: `no_searcher_` (held whenever at least one
searcher is inside) and `no_inserter_` (held whenever the inserter is
inside). Searchers as a *group* hold the first; the inserter holds the
second; a deleter needs both. Add one more semaphore, `insert_mutex_`, so
inserters take turns among themselves.

</details>

<details>
<summary>Hint 2</summary>

"The first searcher in acquires `no_searcher_`; the last one out releases
it" is exactly the **lightswitch** pattern (first into the room turns the
light on, last out turns it off): a counter, a `std::mutex` for the
counter, and the target semaphore. Write it once as a small class,
instantiate it twice — a search switch on `no_searcher_` and an insert
switch on `no_inserter_`. The deleter doesn't need a switch: it just
acquires both semaphores, always in the same order (see the README's
deadlock trap).

</details>

<details>
<summary>Hint 3</summary>

```cpp
class Lightswitch {
  public:
    void lock(std::binary_semaphore& sem) {
        std::lock_guard g(mutex_);
        if (++count_ == 1) sem.acquire();   // first one in locks the door
    }
    void unlock(std::binary_semaphore& sem) {
        std::lock_guard g(mutex_);
        if (--count_ == 0) sem.release();   // last one out unlocks it
    }
  private:
    std::mutex mutex_;
    int count_ = 0;
};
```

- `search_enter`: `search_switch_.lock(no_searcher_)`
- `search_exit`: `search_switch_.unlock(no_searcher_)`
- `insert_enter`: `insert_switch_.lock(no_inserter_)` then
  `insert_mutex_.acquire()`
- `insert_exit`: `insert_mutex_.release()` then
  `insert_switch_.unlock(no_inserter_)`
- `delete_enter`: `no_searcher_.acquire()` then `no_inserter_.acquire()`
- `delete_exit`: release both (reverse order is tidy)

Deadlock check: the deleter is the only thread that ever holds two gate
semaphores at once, and every deleter takes them in the same order — no
cycle. Starvation is another story: a steady stream of searchers keeps
`no_searcher_` lit indefinitely and a deleter can wait forever. The book
accepts that here; koans 11–13 explored the fixes.

</details>
