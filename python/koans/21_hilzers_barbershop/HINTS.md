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

A workable roster: `customers` counter with a `mutex`; a
`sofa = Semaphore(sofa_size)` multiplex; a deque of per-customer channel
pairs (`chair`, `done` — both `Semaphore(0)`) with its own little lock; a
`customer_waiting = Semaphore(0)` the barbers sleep on; `payment(0)` and
`receipt(0)` semaphores plus a `register` mutex so only one barber handles
money at a time. Book note: the book's version also queues customers at
the *door* and lets barbers run that queue too, which it admits leaves the
sofa underutilized (its fix is a separate "usher" thread); here customers
walk to the sofa on their own, which keeps the sofa honest and the koan's
tests satisfiable.

</details>

<details>
<summary>Hint 3</summary>

The staged pseudocode:

```python
# customer_visit(cid)
chair, done = Semaphore(0), Semaphore(0)
with mutex:
    if customers == capacity:
        return False
    customers += 1
hooks.enter_shop(cid)

sofa.acquire()
with queue_lock:                 # seat + enqueue atomically → FIFO is real
    hooks.sit_on_sofa(cid)
    queue.append((chair, done))
customer_waiting.release()
chair.acquire()                  # a barber called *you*
hooks.sit_in_chair(cid)          # the haircut happens here
done.release()
sofa.release()                   # stand up only after sit_in_chair returned

hooks.pay(cid)
payment.release()
receipt.acquire()
with mutex:
    customers -= 1
return True

# each barber (bid)
while True:
    customer_waiting.acquire()
    with queue_lock:
        chair, done = queue.popleft()
    chair.release()              # longest-seated customer first
    hooks.cut_hair(bid)
    done.acquire()               # this cut is fully finished
    payment.acquire()            # somebody has money out
    with register:
        hooks.accept_payment(bid)
    receipt.release()
```

Each barber owns one chair implicitly: it never calls the next customer
until `done` says the current one is finished, which is exactly what caps
haircuts at `n_barbers` while still allowing all of them in parallel.

</details>
