# Hints — Koan 20: FIFO Barbershop

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

"The barber calls *you specifically*" means each customer needs a private
channel to sleep on — one semaphore per waiting customer, not one shared
semaphore for all of them. Who creates it, how does the barber find the
right one, and (this being C++) who keeps it alive?

</details>

<details>
<summary>Hint 2</summary>

Keep a `std::deque<std::shared_ptr<std::binary_semaphore>>` protected by
the same mutex as the `customers` counter. A customer registers by
appending its own freshly-made semaphore inside the mutex, then signals "a
customer is ready" and waits on *its own* semaphore. The barber, once
woken, pops the oldest entry (inside the mutex!) and signals exactly that
one. `shared_ptr` matters: the deque drops its reference on pop while the
customer is still blocked on the object — two owners, two threads, one
lifetime problem solved. The shared `barber` semaphore from koan 19
disappears; the two done-semaphores stay.

</details>

<details>
<summary>Hint 3</summary>

Both sides, in pseudocode:

```cpp
// customer_visit(get_hair_cut)
auto my_turn = std::make_shared<std::binary_semaphore>(0);
{
    std::lock_guard lock(mutex_);
    if (customers_ == n_) return false;
    ++customers_;
    queue_.push_back(my_turn);
}
customer_.release();
my_turn->acquire();        // sleep until called BY NAME
get_hair_cut();
customer_done_.release();
barber_done_.acquire();
{
    std::lock_guard lock(mutex_);
    --customers_;
}
return true;

// barber daemon
while (true) {
    customer_.acquire();
    std::shared_ptr<std::binary_semaphore> next;
    {
        std::lock_guard lock(mutex_);
        next = queue_.front();
        queue_.pop_front();
    }
    next->release();
    cut_hair();
    customer_done_.acquire();
    barber_done_.release();
}
```

Appending to the queue in the same mutex-hold as the capacity check is
what makes "arrival order" well-defined in the first place.

</details>
