# Hints — Koan 10: Bounded buffer

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 09's `items` semaphore counts what's *in* the buffer. The new
constraint is about what's *not yet in* the buffer. That deserves its own
counting semaphore — a mirror image of `items`.

</details>

<details>
<summary>Hint 2</summary>

Three sync members: `mutex = Semaphore(1)`, `items = Semaphore(0)`,
`spaces = Semaphore(capacity)`. A producer consumes a space and creates an
item; a consumer consumes an item and creates a space. Keep every
semaphore wait *outside* the mutex — a producer that sleeps on `spaces`
while holding the mutex locks out the very consumer that would wake it.

</details>

<details>
<summary>Hint 3</summary>

```python
def produce(self, item):
    self.spaces.acquire()
    with self.mutex:
        self.buffer.add(item)
    self.items.release()

def consume(self):
    self.items.acquire()
    with self.mutex:
        item = self.buffer.get()
    self.spaces.release()
    return item
```

Note the symmetry: each side waits for its resource before the mutex and
announces the opposite resource after releasing it.

</details>
