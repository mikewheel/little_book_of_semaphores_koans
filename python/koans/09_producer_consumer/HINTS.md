# Hints — Koan 09: Producer-consumer

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two jobs, two tools. Exclusive access to the buffer wants a mutex (a
`Semaphore(1)` or `threading.Lock`). "Is there anything to consume yet?"
wants a *counting* semaphore whose value tracks the number of items in
the buffer. What should its initial value be?

</details>

<details>
<summary>Hint 2</summary>

The consumer waits on the item-count semaphore *before* taking the mutex.
If you swap those two — wait for an item while holding the mutex — an
empty buffer deadlocks the whole system: the producer needs that mutex to
add the item you're waiting for. Rule of thumb: waiting on a semaphore
while holding a mutex should always make you nervous.

For the producer there is also a small performance question: signal the
item count *inside* the mutex (correct, but the woken consumer may slam
straight into the still-held mutex) or *after* releasing it (kinder to the
scheduler). Both pass; the book prefers the latter.

</details>

<details>
<summary>Hint 3</summary>

With `mutex = Semaphore(1)` and `items = Semaphore(0)`:

```python
def produce(self, item):
    with self.mutex:
        self.buffer.add(item)
    self.items.release()

def consume(self):
    self.items.acquire()
    with self.mutex:
        return self.buffer.get()
```

The one ordering that must hold: the item is *in the buffer* before
`items.release()` announces it. Release first and a consumer can wake,
beat you to the mutex, and find nothing there.

</details>
