# Hints — Koan 19: Barbershop

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two separate problems hide in here. The balk needs a *scoreboard*: a
`customers` counter under a mutex, checked at the door — return `false`
without touching anything else if it reads `n`. The haircut needs
*rendezvouses*: semaphores that pair one barber iteration with one
customer, in both directions.

</details>

<details>
<summary>Hint 2</summary>

The roster: `int customers = 0` with a `std::mutex`, plus four semaphores
all starting at 0 — `customer` ("a customer is ready"), `barber` ("the
barber is ready for you"), `customer_done`, and `barber_done`. That is
*two* rendezvouses: one to start the haircut, one to end it. Read
`customer.acquire()` as "wait for a customer," not "customers wait here."
Skipping the second rendezvous is the classic bug: the barber loops early
and two customers end up mid-haircut at once.

</details>

<details>
<summary>Hint 3</summary>

Both sides, in pseudocode:

```cpp
// customer_visit(get_hair_cut)
{
    std::lock_guard lock(mutex_);
    if (customers_ == n_) return false;
    ++customers_;
}
customer_.release();
barber_.acquire();
get_hair_cut();
customer_done_.release();
barber_done_.acquire();
{
    std::lock_guard lock(mutex_);
    --customers_;
}
return true;

// barber daemon (capture cut_hair by move into the detached thread)
while (true) {
    customer_.acquire();
    barber_.release();
    cut_hair();
    customer_done_.acquire();
    barber_done_.release();
}
```

The counter decrement happens *after* the closing rendezvous, so a seat
frees up only once its haircut is truly over. The lambda captures `this`;
the shop must outlive the detached barber (the tests arrange that by
leaking it).

</details>
