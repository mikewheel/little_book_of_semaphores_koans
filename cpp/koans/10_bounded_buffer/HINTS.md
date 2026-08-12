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

Three sync members: a mutex, `items` starting at 0, and `spaces` starting
at `capacity`. A producer consumes a space and creates an item; a consumer
consumes an item and creates a space. Keep every semaphore wait *outside*
the mutex — a producer that sleeps on `spaces` while holding the mutex
locks out the very consumer that would wake it.

</details>

<details>
<summary>Hint 3</summary>

With `std::mutex mutex_;`, `std::counting_semaphore<> items_{0};` and
`std::counting_semaphore<> spaces_{capacity};` (note: runtime value goes
to the constructor, via an initializer list or member init in the
constructor body's init-list):

```cpp
void produce(int item) {
    spaces_.acquire();
    {
        std::lock_guard lock(mutex_);
        buffer_.add(item);
    }
    items_.release();
}

int consume() {
    items_.acquire();
    int item;
    {
        std::lock_guard lock(mutex_);
        item = buffer_.get();
    }
    spaces_.release();
    return item;
}
```

Note the symmetry: each side waits for its resource before the mutex and
announces the opposite resource after releasing it.

</details>
