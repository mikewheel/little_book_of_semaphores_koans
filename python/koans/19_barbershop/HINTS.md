# Hints — Koan 19: Barbershop

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two separate problems hide in here. The balk needs a *scoreboard*: a
`customers` counter under a mutex, checked at the door — return `False`
without touching anything else if it reads `n`. The haircut needs
*rendezvouses*: semaphores that pair one barber iteration with one
customer, in both directions.

</details>

<details>
<summary>Hint 2</summary>

The roster: `customers = 0` with a `mutex`, plus four semaphores all
starting at 0 — `customer` ("a customer is ready"), `barber` ("the barber
is ready for you"), `customer_done`, and `barber_done`. That is *two*
rendezvouses: one to start the haircut, one to end it. Read
`customer.acquire()` as "wait for a customer," not "customers wait here."
Skipping the second rendezvous is the classic bug: the barber loops early
and two customers end up mid-haircut at once.

</details>

<details>
<summary>Hint 3</summary>

Both sides, in pseudocode:

```python
# customer_visit(get_hair_cut)
with mutex:
    if customers == n:
        return False
    customers += 1
customer.release()
barber.acquire()
get_hair_cut()
customer_done.release()
barber_done.acquire()
with mutex:
    customers -= 1
return True

# barber daemon
while True:
    customer.acquire()
    barber.release()
    cut_hair()
    customer_done.acquire()
    barber_done.release()
```

The counter decrement happens *after* the closing rendezvous, so a seat
frees up only once its haircut is truly over.

</details>
