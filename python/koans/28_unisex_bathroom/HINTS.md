# Hints — Koan 28: Unisex bathroom

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Two ideas compose:

- A semaphore `empty` (initially 1) meaning "the room contains nobody".
  Each *gender as a group* holds it while any member is inside — the
  **lightswitch** from koan 27, one switch per gender.
- A per-gender **multiplex** (koan 04) of size `capacity` to cap how many
  of the current gender squeeze in.

</details>

<details>
<summary>Hint 2</summary>

The full member roster:

```python
self.empty = threading.Semaphore(1)     # 1 while the room has nobody
self.female_switch = Lightswitch()       # women's claim on `empty`
self.male_switch = Lightswitch()
self.female_multiplex = threading.Semaphore(capacity)
self.male_multiplex = threading.Semaphore(capacity)
```

The first woman in locks `empty` (waiting for the room to drain if men
hold it); the last woman out releases it. That is exactly why a waiting
man doesn't slip in when one of two women leaves: the women's lightswitch
still holds `empty` until the count hits zero.

</details>

<details>
<summary>Hint 3</summary>

The female side (male is a mirror image):

```python
def female_enter(self):
    self.female_switch.lock(self.empty)      # first woman claims the room
    self.female_multiplex.acquire()          # then take one of 3 slots

def female_exit(self):
    self.female_multiplex.release()
    self.female_switch.unlock(self.empty)    # last woman returns the room
```

Note the multiplex lives *inside* the lightswitch claim. A fourth woman
blocked on the multiplex is already counted by the switch, which keeps
men out — harsh on the men, but that is this koan's accepted starvation
(and koan 29's opening complaint).

</details>
