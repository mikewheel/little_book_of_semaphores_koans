# Hints — Koan 28: Unisex bathroom

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two ideas compose:

- A semaphore `empty_` (initially 1) meaning "the room contains nobody".
  Each *gender as a group* holds it while any member is inside — the
  **lightswitch** from koan 27, one switch per gender.
- A per-gender **multiplex** (koan 04) of size `capacity` to cap how many
  of the current gender squeeze in.

</details>

<details>
<summary>Hint 2</summary>

The full member roster:

```cpp
std::binary_semaphore empty_{1};        // 1 while the room has nobody
Lightswitch female_switch_;              // women's claim on empty_
Lightswitch male_switch_;
std::counting_semaphore<> female_multiplex_;  // init with capacity
std::counting_semaphore<> male_multiplex_;
```

The first woman in locks `empty_` (waiting for the room to drain if men
hold it); the last woman out releases it. That is exactly why a waiting
man doesn't slip in when one of two women leaves: the women's lightswitch
still holds `empty_` until the count hits zero.

</details>

<details>
<summary>Hint 3</summary>

The female side (male is a mirror image):

```cpp
void female_enter() {
    female_switch_.lock(empty_);     // first woman claims the room
    female_multiplex_.acquire();     // then take one of capacity_ slots
}

void female_exit() {
    female_multiplex_.release();
    female_switch_.unlock(empty_);   // last woman returns the room
}
```

Note the multiplex lives *inside* the lightswitch claim. A fourth woman
blocked on the multiplex is already counted by the switch, which keeps
men out — harsh on the men, but that is this koan's accepted starvation
(and koan 29's opening complaint).

</details>
