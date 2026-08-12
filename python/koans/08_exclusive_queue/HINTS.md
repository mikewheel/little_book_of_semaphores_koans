# Hints — Koan 08: Exclusive queue

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 07 got away without state because nobody needed to *ask* anything —
but here an arriving dancer must know "is a partner already waiting?",
and a semaphore cannot be asked its value. So keep a scoreboard: two
counters (waiting leaders, waiting followers) guarded by a mutex. An
arriving dancer either claims a waiting partner (decrement their
counter, release their queue) or increments its own counter and parks on
its own queue.

</details>

<details>
<summary>Hint 2</summary>

The exclusivity comes from an **asymmetric hand-off of the mutex**: the
dancer who completes the pair does *not* release the mutex — the pair
keeps it for the whole dance, so no other pair can even reach the
scoreboard. The leader releases it at the very end, after a final
rendezvous semaphore tells it the partner's dance has finished. Note
what that implies: the mutex may be acquired by one thread and released
by a different one. (A dancer who parks must release the mutex *before*
sleeping on its queue, or the ballroom deadlocks — koan 05's lesson.)

</details>

<details>
<summary>Hint 3</summary>

Book-style solution, both sides:

```python
def __init__(self):
    self.leaders = 0
    self.followers = 0
    self.mutex = threading.Semaphore(1)
    self.leader_queue = threading.Semaphore(0)
    self.follower_queue = threading.Semaphore(0)
    self.rendezvous = threading.Semaphore(0)

def leader_dances(self, dance):
    self.mutex.acquire()
    if self.followers > 0:
        self.followers -= 1
        self.follower_queue.release()   # claim a waiting partner
    else:
        self.leaders += 1
        self.mutex.release()            # let go before parking!
        self.leader_queue.acquire()     # woken by a follower who kept it
    dance()
    self.rendezvous.acquire()           # partner has finished dancing
    self.mutex.release()                # the pair's mutex, whoever took it

def follower_dances(self, dance):
    self.mutex.acquire()
    if self.leaders > 0:
        self.leaders -= 1
        self.leader_queue.release()
    else:
        self.followers += 1
        self.mutex.release()
        self.follower_queue.acquire()
    dance()
    self.rendezvous.release()           # tell the leader we're done
```

Exactly one dancer per pair takes the mutex and keeps it through the
dance; the leader always gives it back. That is why only one pair can be
on the floor, and why the leader cannot leave early.

</details>
