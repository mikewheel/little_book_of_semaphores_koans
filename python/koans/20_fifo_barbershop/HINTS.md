# Hints — Koan 20: FIFO Barbershop

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

"The barber calls *you specifically*" means each customer needs a private
channel to sleep on — one semaphore per waiting customer, not one shared
semaphore for all of them. Who creates it, and how does the barber find
the right one?

</details>

<details>
<summary>Hint 2</summary>

Keep a queue (a `deque`) of those per-customer semaphores, protected by the
same mutex as the `customers` counter. A customer registers by appending
its own fresh `Semaphore(0)` inside the mutex, then signals "a customer is
ready" and waits on *its own* semaphore. The barber, once woken, pops the
oldest semaphore (inside the mutex!) and signals exactly that one. The
shared `barber` semaphore from koan 19 disappears; the two done-semaphores
stay.

</details>

<details>
<summary>Hint 3</summary>

Both sides, in pseudocode:

```python
# customer_visit(get_hair_cut)
my_turn = threading.Semaphore(0)
with mutex:
    if customers == n:
        return False
    customers += 1
    queue.append(my_turn)
customer.release()
my_turn.acquire()          # sleep until called BY NAME
get_hair_cut()
customer_done.release()
barber_done.acquire()
with mutex:
    customers -= 1
return True

# barber daemon
while True:
    customer.acquire()
    with mutex:
        next_turn = queue.popleft()
    next_turn.release()
    cut_hair()
    customer_done.acquire()
    barber_done.release()
```

Appending to the queue in the same mutex-hold as the capacity check is
what makes "arrival order" well-defined in the first place.

</details>
