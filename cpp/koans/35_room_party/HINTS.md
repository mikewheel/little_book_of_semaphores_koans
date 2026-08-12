# Hints — Koan 35: Room party

Stop! Try it yourself first. Each hint below gives away more than the last.
This is one of the hardest koans in the collection — budget real time
before peeking.

<details>
<summary>Hint 1</summary>

Scoreboard first: a `students` count and a `dean` state with three values —
*not here*, *waiting*, *in the room* — all protected by one mutex. Every
entry, exit, and dean decision reads and writes this scoreboard under the
mutex. The dean's three cases on arrival fall straight out of the rules:
empty → search; over threshold → break up; otherwise → set state to
*waiting* and sleep. The hard part is who wakes him and what they promise.

</details>

<details>
<summary>Hint 2</summary>

Four semaphores next to the scoreboard: the mutex (a `binary_semaphore` —
you'll see why in a moment); a turnstile the dean locks while in the room
(entering students wait there, leavers never touch it); and two rendezvous
signals — one a student uses to wake a waiting dean (fired by whichever
student makes the count hit `threshold + 1`, or by the last one out), and
one the last student out fires when the dean is already inside waiting for
the room to clear.

The crucial discipline is **pass the baton**: a student who signals the
dean does *not* release the mutex — ownership transfers with the signal,
so the dean wakes already holding it. (This is why the mutex must be a
semaphore: `std::mutex` forbids cross-thread unlock.) That is how the dean
can trust the scoreboard he wakes to: nobody can move the counts between
the signal and his re-check.

</details>

<details>
<summary>Hint 3</summary>

```text
students = 0; dean = NOT_HERE
mutex = binary_semaphore(1); turn = binary_semaphore(1)
lie_in = binary_semaphore(0)  // student wakes a waiting dean (baton passes)
clear  = binary_semaphore(0)  // last student out tells the in-room dean (baton)

dean_visit:
    mutex.acquire();
    if (0 < students && students <= threshold) {
        dean = WAITING; mutex.release();
        lie_in.acquire();           // wake holding the baton
    }
    // now students == 0 or students > threshold
    if (students > threshold) {
        dean = IN_ROOM; breakup();
        turn.acquire();             // bar the door
        mutex.release();
        clear.acquire();            // baton back from the last leaver
        turn.release();
    } else {
        search();
    }
    dean = NOT_HERE; mutex.release();

student_visit(sid):
    mutex.acquire();
    if (dean == IN_ROOM) {
        mutex.release(); turn.acquire(); turn.release(); mutex.acquire();
    }
    ++students;
    if (students == threshold + 1 && dean == WAITING)
        lie_in.release();           // baton to the dean
    else
        mutex.release();

    party(sid);

    mutex.acquire();
    --students;
    if (students == 0 && dean == WAITING)      lie_in.release();  // baton
    else if (students == 0 && dean == IN_ROOM) clear.release();   // baton
    else                                       mutex.release();
```

Why the dean's re-check is sound: he reaches it either because his first
test failed (so the count was 0 or past threshold, and he never let go of
the mutex) or because a student signaled `lie_in` — and both signal sites
fire precisely when the count is 0 or `threshold + 1`, with the mutex
passed along so nothing can change in between.

</details>
