# Hints — Koan 30: Baboon crossing

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

This is the no-starve unisex bathroom wearing a rope costume: two
categories that must not mix, a hard cap on simultaneous occupants, full
sharing within a category, and no starvation. Compose the same three
gadgets — a **lightswitch** per direction, a **turnstile** in front of
both, and a **multiplex** for the cap.

</details>

<details>
<summary>Hint 2</summary>

A workable roster:

- `rope_free = Semaphore(1)` — held by whichever direction owns the rope.
- one lightswitch per direction — first one on locks `rope_free`, last
  one off releases it.
- `turnstile = Semaphore(1)` — shared by both directions; a baboon stuck
  waiting for `rope_free` waits *inside* it, stalling all later arrivals.
- `multiplex = Semaphore(capacity)` — the 5-baboon weight limit. One
  shared multiplex is enough: only one direction is ever on the rope, and
  exiting baboons return their token before the rope changes hands.

</details>

<details>
<summary>Hint 3</summary>

Eastbound (westbound is symmetric):

```python
def east_enter(self):
    self.turnstile.acquire()
    self.east_switch.lock(self.rope_free)  # first eastbound claims the rope
    self.turnstile.release()
    self.multiplex.acquire()               # weight limit

def east_exit(self):
    self.multiplex.release()
    self.east_switch.unlock(self.rope_free)  # last one off frees the rope
```

While eastbound baboons own the rope, more of them stream through the
turnstile and are throttled only by the multiplex. The first westbound
arrival blocks inside the turnstile holding it, so nobody else gets past;
when the eastbound cohort drains, it claims the rope, and the eastbound
baboons queued behind it get their turn afterwards.

</details>
