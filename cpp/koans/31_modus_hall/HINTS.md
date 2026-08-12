# Hints — Koan 31: The Modus Hall problem

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

This is a **scoreboard** problem. Keep counts of *both* factions'
checked-in members, plus a status field with five values: neutral,
heathens rule, prudes rule, transition to heathens, transition to prudes
(an `enum class` and a `switch`). Every arrival and every departure is a
case analysis on that status, under one mutex. Decide what each case does
before you touch a semaphore.

</details>

<details>
<summary>Hint 2</summary>

A roster that works:

- `int heathens_, prudes_` — checked-in counts (waiting *and* crossing).
- `Status status_` — the five-state enum from Hint 1.
- `std::mutex mutex_` — guards all of the above.
- `std::binary_semaphore heathen_turn_{1}, prude_turn_{1}` — one
  turnstile per faction, at the door. Locking `prude_turn_` bars new
  prudes during a transition to heathen control, and vice versa.
- `std::counting_semaphore<> heathen_queue_{0}, prude_queue_{0}` — where
  checked-in members wait for their faction's turn; released as a whole
  batch with `release(n)`.

Arrivals pass their own turnstile (acquire, then immediately release),
check in under the mutex, then either cross or wait on their queue. The
arrival that tips the majority sets the status to "transition" and locks
the *opponents'* turnstile. Departures decrement, and the *last* one out
hands the path to the waiting cohort — releasing the queue `n` times —
and reopens whichever turnstile the transition had locked.

</details>

<details>
<summary>Hint 3</summary>

Heathen check-in and check-out (prudes are the mirror image):

```cpp
heathen_turn_.acquire(); heathen_turn_.release();   // the door

mutex_.lock();
++heathens_;
switch (status_) {
  case Status::Neutral:
    status_ = Status::HeathensRule;  mutex_.unlock();  break;
  case Status::PrudesRule:
    if (heathens_ > prudes_) {            // we just tipped it
        status_ = Status::TransitionToHeathens;
        prude_turn_.acquire();            // bar new prudes at their door
    }
    mutex_.unlock();  heathen_queue_.acquire();  break;
  case Status::TransitionToHeathens:
    mutex_.unlock();  heathen_queue_.acquire();  break;
  case Status::HeathensRule:
  case Status::TransitionToPrudes:
    mutex_.unlock();  break;              // pass freely
}

cross();

mutex_.lock();
--heathens_;
if (heathens_ == 0) {                     // last one out flips the field
    if (status_ == Status::TransitionToPrudes)
        heathen_turn_.release();          // reopen the door the transition locked
    if (prudes_ > 0) { prude_queue_.release(prudes_); status_ = Status::PrudesRule; }
    else             { status_ = Status::Neutral; }
} else if (status_ == Status::HeathensRule && prudes_ > heathens_) {
    status_ = Status::TransitionToPrudes; // our departure tipped it
    heathen_turn_.acquire();
}
mutex_.unlock();
```

That `prude_turn_.acquire()` under the held mutex is safe: the only
long-term holder of a turnstile is a transition, and transitions of that
flavor can't be in progress in the branch that acquires it — any other
holder is a passer-through who releases immediately and holds no lock.

Two book notes. First, its own caveat: threads that have passed the
turnstile but not yet checked in aren't counted — majority is over
*registered voters*, and the tests honor that. Second, if you compare
with the book's printed solution: its last-one-out branch signals the
wrong turnstile — reopen the one the transition actually locked, as
above, or a faction stays locked out forever.

</details>
