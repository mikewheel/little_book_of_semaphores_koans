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

- `std::binary_semaphore rope_free_{1}` — held by whichever direction
  owns the rope.
- one `Lightswitch` per direction (counter + `std::mutex`) — first one on
  locks `rope_free_`, last one off releases it.
- `std::binary_semaphore turnstile_{1}` — shared by both directions; a
  baboon stuck waiting for the rope waits *inside* it, stalling all later
  arrivals.
- `std::counting_semaphore<> multiplex_` initialized to `capacity` — the
  5-baboon weight limit. One shared multiplex is enough: only one
  direction ever draws tokens, and exiting baboons return their token
  before the rope changes hands.

</details>

<details>
<summary>Hint 3</summary>

Eastbound (westbound is symmetric):

```cpp
void east_enter() {
    turnstile_.acquire();
    east_switch_.lock(rope_free_);  // first eastbound claims the rope
    turnstile_.release();
    multiplex_.acquire();           // weight limit
}

void east_exit() {
    multiplex_.release();
    east_switch_.unlock(rope_free_);  // last one off frees the rope
}
```

While eastbound baboons own the rope, more of them stream through the
turnstile and are throttled only by the multiplex. The first westbound
arrival blocks inside the turnstile holding it, so nobody else gets past;
when the eastbound cohort drains, it claims the rope, and the eastbound
baboons queued behind it get their turn afterwards.

</details>
