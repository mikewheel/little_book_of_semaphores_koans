# Hints — Koan 27: Search-Insert-Delete

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Think in terms of two "coast is clear" flags, each a semaphore initialized
to 1: `no_searcher` (held whenever at least one searcher is inside) and
`no_inserter` (held whenever the inserter is inside). Searchers as a
*group* hold the first; the inserter holds the second; a deleter needs
both. Add one more plain semaphore, `insert_mutex`, so inserters take
turns among themselves.

</details>

<details>
<summary>Hint 2</summary>

"The first searcher in acquires `no_searcher`; the last one out releases
it" is exactly the **lightswitch** pattern (first into the room turns the
light on, last out turns it off): a counter, a lock for the counter, and
the target semaphore. Build it once, instantiate it twice — a search
switch on `no_searcher` and an insert switch on `no_inserter`. The
deleter doesn't need a switch: it just acquires both semaphores, always
in the same order (see the README's deadlock trap).

</details>

<details>
<summary>Hint 3</summary>

```python
class Lightswitch:
    def __init__(self):
        self.count = 0
        self.mutex = threading.Lock()

    def lock(self, semaphore):
        with self.mutex:
            self.count += 1
            if self.count == 1:
                semaphore.acquire()   # first one in locks the door

    def unlock(self, semaphore):
        with self.mutex:
            self.count -= 1
            if self.count == 0:
                semaphore.release()   # last one out unlocks it
```

- search_enter: `search_switch.lock(no_searcher)`
- search_exit: `search_switch.unlock(no_searcher)`
- insert_enter: `insert_switch.lock(no_inserter)` then `insert_mutex.acquire()`
- insert_exit: `insert_mutex.release()` then `insert_switch.unlock(no_inserter)`
- delete_enter: `no_searcher.acquire()` then `no_inserter.acquire()`
- delete_exit: release both (reverse order is tidy)

Deadlock check: the deleter is the only thread that ever holds two of the
gate semaphores, and every deleter takes them in the same order — so no
cycle. Starvation is another story: a steady stream of searchers keeps
`no_searcher` lit indefinitely and the deleter can wait forever. The book
accepts that for this problem; the same tension (and its fixes) showed up
in the readers-writers koans.

</details>
