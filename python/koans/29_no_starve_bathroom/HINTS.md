# Hints — Koan 29: No-starve unisex bathroom

Stop! Try it yourself first. Each hint below gives away more than the last.

<details>
<summary>Hint 1</summary>

Koan 12 (no-starve readers-writers) fixed the same disease with the same
medicine: a **turnstile** that a waiting thread can stall while it stands
inside, so everyone who arrives later — friend or foe — queues up behind
it. What would the turnstile guard here, and how many turnstiles do the
two genders need?

</details>

<details>
<summary>Hint 2</summary>

One shared turnstile, in front of *both* genders' entrances. Keep the
whole koan-28 roster (an `empty` semaphore, one lightswitch per gender,
one capacity multiplex per gender) and add a single `turnstile =
Semaphore(1)`. On entry: pass the turnstile, lock your gender's
lightswitch, and only then release the turnstile. A man who blocks waiting
for the room to empty is standing *inside* the turnstile — late women pile
up behind him instead of slipping past.

</details>

<details>
<summary>Hint 3</summary>

The male side (female is symmetric):

```python
def male_enter(self):
    self.turnstile.acquire()
    self.male_switch.lock(self.empty)   # first man in claims the room
    self.turnstile.release()
    self.male_multiplex.acquire()       # capacity cap

def male_exit(self):
    self.male_multiplex.release()
    self.male_switch.unlock(self.empty)  # last man out frees the room
```

While men occupy the room, more men flow through the turnstile freely. The
moment a woman arrives she blocks inside the turnstile (holding it), so no
one else — of either gender — gets past until the room empties and she
claims it. That is exactly constraint 4.

</details>
