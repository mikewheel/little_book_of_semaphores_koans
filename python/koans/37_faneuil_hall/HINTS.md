# Hints — Koan 37: Faneuil Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

One semaphore can play two parts at once: it is the *turnstile* everyone
passes through to enter, and it is the thing the judge holds shut for her
whole visit. Call it `no_judge`, initially 1.

Alongside it, keep a scoreboard: `entered` (immigrants who came in this
ceremony) and `checked` (immigrants who have checked in), the latter
guarded by a `mutex` (a `Semaphore(1)` is fine). The judge can confirm
exactly when `entered == checked`.

</details>

<details>
<summary>Hint 2</summary>

The judge takes **both** `no_judge` and `mutex` on arrival. If
`entered > checked` she cannot confirm yet — so she releases the mutex
and sleeps on an `all_signed_in` semaphore. The **last immigrant to check
in** (the one who makes `checked` catch up to `entered` while a judge is
present) signals `all_signed_in` *without releasing the mutex*: the mutex
is handed directly to the judge. That is the pass-the-baton pattern.

For the confirmation itself: every seated immigrant is parked on a
`confirmed` semaphore (initially 0). The judge broadcasts with
`confirmed.release(checked)` — and afterwards resets both counters on the
immigrants' behalf ("I'll do it for you"), so watch out: with zero
check-ins there is nothing to release, and `release(0)` raises.

</details>

<details>
<summary>Hint 3</summary>

Immigrant:

```python
no_judge.acquire()
enter(); entered += 1
no_judge.release()

mutex.acquire()
check_in(); checked += 1
if judge_present and entered == checked:
    all_signed_in.release()        # baton: the mutex goes with it
else:
    mutex.release()

sit_down()
confirmed.acquire()
swear(); get_certificate()

no_judge.acquire()                 # can't leave while the judge is in
leave()
no_judge.release()
```

Judge:

```python
no_judge.acquire(); mutex.acquire()
enter(); judge_present = True
if entered > checked:
    mutex.release()
    all_signed_in.acquire()        # baton comes back with the mutex
confirm()
if checked:
    confirmed.release(checked)
entered = checked = 0
leave(); judge_present = False
mutex.release(); no_judge.release()
```

Spectators only interact with the turnstile: acquire `no_judge`, enter,
release; spectate and leave with no further ceremony.

</details>
