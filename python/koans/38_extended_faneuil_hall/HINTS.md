# Hints — Koan 38: Extended Faneuil Hall

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Start from the koan 37 solution. The tempting fix — judge releases
`no_judge` after her `leave` so immigrants can file out — reopens the
front door to everyone, including her own next visit. She must keep the
building locked and let the sworn immigrants out through a *different*
channel, only releasing `no_judge` once the last of them is gone.

Think: an exit-side baton.

</details>

<details>
<summary>Hint 2</summary>

Two new semaphores, both starting at 0: an `exit` turnstile and
`all_gone`. After her `leave`, the judge signals `exit` once — that
signal carries the mutex with it (pass the baton) — and then sleeps on
`all_gone`, still holding `no_judge`.

Each departing immigrant wakes holding the baton, fires her `leave`, and
counts herself out by decrementing `checked` (the same counter that was
counted up at check-in — so the judge must *not* reset it this time; only
`entered` resets). If others remain she passes the baton back to `exit`;
the last one out signals `all_gone` instead, handing the mutex back to
the judge, who finally releases `mutex` and `no_judge`.

One wrinkle: if the ceremony had zero immigrants there is nobody to
signal `all_gone` — skip the exit choreography entirely in that case.

</details>

<details>
<summary>Hint 3</summary>

Immigrant, from the certificate onward (everything before is koan 37):

```python
swear(); get_certificate()

exit.acquire()                  # baton arrives carrying the mutex
leave()
checked -= 1
if checked == 0:
    all_gone.release()          # baton back to the judge
else:
    exit.release()              # baton to the next immigrant
```

Judge:

```python
no_judge.acquire(); mutex.acquire()
enter(); judge_present = True
if entered > checked:
    mutex.release(); all_signed_in.acquire()
confirm()
if checked:
    confirmed.release(checked)
entered = 0                     # checked now counts down at the exit
leave(); judge_present = False
if checked:
    exit.release()              # start the drain (baton + mutex)
    all_gone.acquire()          # wait for the last one out
mutex.release(); no_judge.release()
```

The judge holds `no_judge` across the whole drain, so her own next visit
(and everyone else) stays outside until the building is empty.

</details>
