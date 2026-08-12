# Hints — Koan 21: Hilzer's Barbershop

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Don't invent anything new — compose. The door is koan 19's scoreboard
(counter + mutex + balk). The sofa is koan 04's multiplex. Sofa-to-chair in
seating order is koan 20's queue of private semaphores. The haircut and the
payment are rendezvouses. Solve each stage on its own, then chain them.

</details>

<details>
<summary>Hint 2</summary>

A workable roster: `int customers_` with a `std::mutex`; a
`std::counting_semaphore<> sofa_{sofa_size}` multiplex; a
`std::deque<std::shared_ptr<Channel>>` (where `Channel` holds two
`binary_semaphore{0}`s: `chair` and `done`) with its own little lock; a
`customer_waiting_{0}` semaphore the barbers sleep on; `payment_{0}` and
`receipt_{0}` semaphores plus a `register_` mutex so only one barber
handles money at a time. Book note: the book's version also queues
customers at the *door* and lets barbers run that queue too, which it
admits leaves the sofa underutilized (its fix is a separate "usher"
thread); here customers walk to the sofa on their own, which keeps the
sofa honest and the koan's tests satisfiable.

</details>

<details>
<summary>Hint 3</summary>

The staged pseudocode:

```cpp
// customer_visit(cid)
auto ch = std::make_shared<Channel>();   // chair{0}, done{0}
{
    std::lock_guard lock(mutex_);
    if (customers_ == capacity_) return false;
    ++customers_;
}
hooks_.enter_shop(cid);

sofa_.acquire();
{
    std::lock_guard lock(queue_lock_);   // seat + enqueue atomically
    hooks_.sit_on_sofa(cid);
    queue_.push_back(ch);
}
customer_waiting_.release();
ch->chair.acquire();                     // a barber called *you*
hooks_.sit_in_chair(cid);                // the haircut happens here
ch->done.release();
sofa_.release();                         // stand up only after the hook returned

hooks_.pay(cid);
payment_.release();
receipt_.acquire();
{
    std::lock_guard lock(mutex_);
    --customers_;
}
return true;

// each barber (bid) — a detached thread
while (true) {
    customer_waiting_.acquire();
    std::shared_ptr<Channel> ch;
    {
        std::lock_guard lock(queue_lock_);
        ch = queue_.front();
        queue_.pop_front();
    }
    ch->chair.release();                 // longest-seated customer first
    hooks_.cut_hair(bid);
    ch->done.acquire();                  // this cut is fully finished
    payment_.acquire();                  // somebody has money out
    {
        std::lock_guard lock(register_);
        hooks_.accept_payment(bid);
    }
    receipt_.release();
}
```

Each barber owns one chair implicitly: it never calls the next customer
until `done` says the current one is finished, which is exactly what caps
haircuts at `n_barbers` while still allowing all of them in parallel.

</details>
