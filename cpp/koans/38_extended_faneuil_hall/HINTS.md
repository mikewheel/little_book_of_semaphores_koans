# Hints — Koan 38: Extended Faneuil Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Start from the koan 37 solution. The tempting fix — judge releases
`no_judge_` after her `leave` so immigrants can file out — reopens the
front door to everyone, including her own next visit. She must keep the
building locked and let the sworn immigrants out through a *different*
channel, only releasing `no_judge_` once the last of them is gone.

Think: an exit-side baton.

</details>

<details>
<summary>Hint 2</summary>

Two new semaphores, both starting at 0: an `exit_` turnstile and
`all_gone_`. After her `leave`, the judge signals `exit_` once — that
signal carries the mutex with it (pass the baton) — and then sleeps on
`all_gone_`, still holding `no_judge_`.

Each departing immigrant wakes holding the baton, fires her `leave`, and
counts herself out by decrementing `checked_` (the same counter that was
counted up at check-in — so the judge must *not* reset it this time; only
`entered_` resets). If others remain she passes the baton back to
`exit_`; the last one out signals `all_gone_` instead, handing the mutex
back to the judge, who finally releases `mutex_` and `no_judge_`.

One wrinkle: if the ceremony had zero immigrants there is nobody to
signal `all_gone_` — skip the exit choreography entirely in that case.

</details>

<details>
<summary>Hint 3</summary>

New members: `std::counting_semaphore<> exit_{0}, all_gone_{0};`.

Immigrant, from the certificate onward (everything before is koan 37):

```cpp
hooks_.swear(iid);
hooks_.get_certificate(iid);

exit_.acquire();                 // baton arrives carrying the mutex
hooks_.leave(...);
--checked_;
if (checked_ == 0)
    all_gone_.release();         // baton back to the judge
else
    exit_.release();             // baton to the next immigrant
```

Judge:

```cpp
no_judge_.acquire(); mutex_.acquire();
hooks_.enter("judge"); judge_present_ = true;
if (entered_ > checked_) { mutex_.release(); all_signed_in_.acquire(); }
hooks_.confirm();
if (checked_ > 0) confirmed_.release(checked_);
entered_ = 0;                    // checked_ now counts down at the exit
hooks_.leave("judge"); judge_present_ = false;
if (checked_ > 0) {
    exit_.release();             // start the drain (baton + mutex)
    all_gone_.acquire();         // wait for the last one out
}
mutex_.release(); no_judge_.release();
```

The judge holds `no_judge_` across the whole drain, so her own next
visit (and everyone else) stays outside until the building is empty.

</details>
