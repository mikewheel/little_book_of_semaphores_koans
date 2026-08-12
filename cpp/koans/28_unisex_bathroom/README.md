# Koan 28 — Unisex bathroom

*Adapted from* The Little Book of Semaphores, *§6.2 (CC BY-NC-SA 4.0).*

## The problem

An office building has one convenient bathroom, and management agrees to
make it unisex — under strict conditions:

- Men and women are never inside at the same time.
- At most `capacity` (classically 3) people are inside at once.
- Up to `capacity` people of the *same* gender may share — a solution
  that admits one person at a time is over-locked and unacceptable.
- No deadlock.

Starvation, for now, is allowed: an unbroken parade of women may keep men
waiting indefinitely (koan 29 repairs that injustice). One more subtlety
worth reading twice: when the bathroom holds two women and a man is
waiting, a woman leaving does **not** let him in — he enters only when
the room is completely *empty*.

## Your task

Edit `bathroom.hpp`. Implement `Bathroom` (constructed with
`capacity = 3`) with:

- `female_enter()` / `female_exit()` — entry blocks while any man is
  inside or while `capacity` women already are.
- `male_enter()` / `male_exit()` — symmetric.

Run: `./check cpp 28`

## Traps worth savoring

- A plain multiplex of size 3 with no notion of gender: capacity is
  respected, and the genders happily mingle. The tests snapshot occupancy
  at every entry and will list the mixed company.
- A single "the room is mine" semaphore per entrant: genders never mix,
  but the second woman blocks behind the first. The sharing test catches
  this over-serialization.
- Letting a waiting man in the moment a slot frees rather than when the
  room empties — the trap called out above; there is a dedicated test.

## Modern C++ notes (many ways to skin this cat)

- The counter-plus-first-in-locks helper (the *lightswitch*) is worth
  writing as a real class this time — koan 27 needed two of them, this
  koan needs two more, and koan 29 will want them again. A member
  `Lightswitch` holding `std::mutex` + `int`, whose `lock(sem)` /
  `unlock(sem)` take the target `std::binary_semaphore&`, composes
  cleanly.
- The runtime `capacity` forces `std::counting_semaphore<>` with its
  default (huge) compile-time ceiling — the template argument is a
  *maximum*, the constructor argument the initial count. Declaring
  `counting_semaphore<3>` would be neat documentation but only works if
  capacity were fixed at compile time.
- Paired `enter`/`exit` calls are exception bait: one throw in between
  and the room is wedged forever. Production API design wraps the pair in
  an RAII guard:

  ```cpp
  class FemaleGuard {
    public:
      explicit FemaleGuard(Bathroom& b) : b_(b) { b_.female_enter(); }
      ~FemaleGuard() { b_.female_exit(); }
      FemaleGuard(const FemaleGuard&) = delete;
      FemaleGuard& operator=(const FemaleGuard&) = delete;
    private:
      Bathroom& b_;
  };
  ```

  The same idea gives the standard library `std::lock_guard`,
  `std::shared_lock`, and friends. The koan keeps raw pairs so the tests
  can hold the room in exotic states — but notice how unnatural that
  would be to misuse through a guard.
