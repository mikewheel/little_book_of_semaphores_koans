# Hints — Koan 09: Producer-consumer

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two jobs, two tools. Exclusive access to the buffer wants a mutex
(`std::mutex`, or a `std::binary_semaphore{1}` if you want to stay in
semaphore-land). "Is there anything to consume yet?" wants a
`std::counting_semaphore` whose value tracks the number of items in the
buffer. What should its initial value be?

</details>

<details>
<summary>Hint 2</summary>

The consumer waits on the item-count semaphore *before* taking the mutex.
If you swap those two — wait for an item while holding the mutex — an
empty buffer deadlocks the whole system: the producer needs that mutex to
add the item you're waiting for. Rule of thumb: blocking on a semaphore
while holding a mutex should always make you nervous.

For the producer there is also a small performance question: release the
item count *inside* the critical section (correct, but the woken consumer
may slam straight into the still-held mutex) or *after* it (kinder to the
scheduler). Both pass; the book prefers the latter.

</details>

<details>
<summary>Hint 3</summary>

With `std::mutex mutex_;` and `std::counting_semaphore<> items_{0};`:

```cpp
void produce(int item) {
    {
        std::lock_guard lock(mutex_);
        buffer_.add(item);
    }
    items_.release();
}

int consume() {
    items_.acquire();
    std::lock_guard lock(mutex_);
    return buffer_.get();
}
```

The one ordering that must hold: the item is *in the buffer* before
`items_.release()` announces it. Release first and a consumer can wake,
beat you to the mutex, and find nothing there.

</details>
