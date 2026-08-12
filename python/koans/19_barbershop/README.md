# Koan 19 — Barbershop

*Adapted from* The Little Book of Semaphores, *§5.2 (CC BY-NC-SA 4.0).*

## The problem

Dijkstra's sleeping barber. A shop holds at most `n` customers — the
waiting seats plus the barber's chair. One barber works there:

- With nobody around, the barber dozes in his own chair.
- An arriving customer who finds the shop full turns around and leaves —
  a **balk** — immediately, without waiting.
- Otherwise the customer takes a seat; if the barber is asleep, the arrival
  wakes him.
- Each haircut involves both parties at once: while the barber runs
  `cut_hair()`, exactly one customer must be running `get_hair_cut()`, and
  the barber may not start on anyone else until that customer's cut is
  completely finished.

Nothing here promises first-come-first-served — any waiting customer may be
called next. (Koan 20 adds that guarantee.)

## Your task

Edit `barbershop.py`. Implement `Barbershop(n)` with:

- `start_barber(cut_hair)` — spawn the barber as a daemon thread: loop
  forever, sleeping until a customer is present, then run `cut_hair()`
  paired 1:1 with that one customer's `get_hair_cut()`, finishing this
  customer fully before taking the next.
- `customer_visit(get_hair_cut) -> bool` — return `False` immediately (no
  blocking) if `n` customers are already in the shop; otherwise wait for
  your turn, run `get_hair_cut()` concurrently with the barber's
  `cut_hair()`, and return `True` only once both sides of the cut are done.

## Traps worth savoring

- Checking "is the shop full?" anywhere but inside your mutual exclusion
  invites two customers to squeeze through the same last seat.
- The subtle classic: signaling the customer to take the chair and then
  cutting, *without* waiting to hear the customer is done. The barber
  finishes `cut_hair()`, loops around, and starts on the next head while
  the previous customer is still in a chair — the tests watch for exactly
  that overlap.

## Python notes

Balking is reported by return value (`False`), which keeps the caller's
control flow boring — the book's version has `balk()` never return, closer
to an exception. Either is defensible in Python; an exception shines when
balking is exceptional, a `bool` when it's a normal outcome the caller
tallies (as our tests do). Also note the two done-signals at the end of a
cut form their own tiny rendezvous — the same shape as koan 02, just
embedded in a bigger protocol.

Run: `./check python 19`
