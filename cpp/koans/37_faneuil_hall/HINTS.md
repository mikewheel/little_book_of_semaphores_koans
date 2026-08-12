# Hints — Koan 37: Faneuil Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

One semaphore can play two parts at once: it is the *turnstile* everyone
passes through to enter, and it is the thing the judge holds shut for her
whole visit. Call it `no_judge_`, initially 1.

Alongside it, keep a scoreboard: `entered_` (immigrants who came in this
ceremony) and `checked_` (immigrants who have checked in), the latter
guarded by a `mutex_` (a semaphore initialized to 1 works). The judge can
confirm exactly when `entered_ == checked_`.

</details>

<details>
<summary>Hint 2</summary>

The judge takes **both** `no_judge_` and `mutex_` on arrival. If
`entered_ > checked_` she cannot confirm yet — so she releases the mutex
and sleeps on an `all_signed_in_` semaphore. The **last immigrant to
check in** (the one who makes `checked_` catch up while a judge is
present) signals `all_signed_in_` *without releasing the mutex*: the
mutex is handed directly to the judge. That is the pass-the-baton
pattern.

For the confirmation itself: every seated immigrant is parked on a
`confirmed_` semaphore (initially 0). The judge broadcasts with
`confirmed_.release(checked_)` and then resets both counters on the
immigrants' behalf ("I'll do it for you"). Guard the zero-immigrant case.

</details>

<details>
<summary>Hint 3</summary>

Members: `std::counting_semaphore<> no_judge_{1}, mutex_{1},
all_signed_in_{0}, confirmed_{0};` plus `int entered_ = 0, checked_ = 0;`
and `bool judge_present_ = false;`.

Immigrant:

```cpp
no_judge_.acquire();
hooks_.enter(...); ++entered_;
no_judge_.release();

mutex_.acquire();
hooks_.check_in(iid); ++checked_;
if (judge_present_ && entered_ == checked_)
    all_signed_in_.release();      // baton: the mutex goes with it
else
    mutex_.release();

hooks_.sit_down(iid);
confirmed_.acquire();
hooks_.swear(iid);
hooks_.get_certificate(iid);

no_judge_.acquire();               // can't leave while the judge is in
hooks_.leave(...);
no_judge_.release();
```

Judge:

```cpp
no_judge_.acquire(); mutex_.acquire();
hooks_.enter("judge"); judge_present_ = true;
if (entered_ > checked_) {
    mutex_.release();
    all_signed_in_.acquire();      // baton comes back with the mutex
}
hooks_.confirm();
if (checked_ > 0) confirmed_.release(checked_);
entered_ = checked_ = 0;
hooks_.leave("judge"); judge_present_ = false;
mutex_.release(); no_judge_.release();
```

Spectators only touch the turnstile: acquire `no_judge_`, enter, release;
then spectate and leave with no further ceremony.

</details>
