# Koan 16 — Cigarette smokers

*Adapted from* The Little Book of Semaphores, *§4.5 (CC BY-NC-SA 4.0).*

## The problem

Patil's 1971 puzzle, in the version Parnas made respectable. Rolling a
cigarette takes three ingredients: tobacco, paper, and a match. Three
smokers sit around a table, each with an infinite supply of exactly *one*
ingredient. An **agent** (think: an operating system's resource
allocator) repeatedly places two *different* ingredients on the table,
then waits to be told a cigarette got smoked. The smoker owning the third
ingredient must pick both up, roll, smoke, and signal the agent.

The agent's code is untouchable — it signals two ingredient semaphores
and waits on `agent_sem`, and it neither knows nor cares who is
listening. (Versions where the agent signals the right smoker directly
are considered cheating and, worse, boring.) In this koan the *tests*
play the agent, using the semaphores in the provided `AgentTable`.

Contract, per round:

- Exactly one smoker smokes — the one whose own ingredient completes the
  pair on the table. `on_smoke(kind)` reports the smoking smoker's *own*
  ingredient.
- Both placed ingredients are consumed by that smoker; nobody else
  touches anything.
- After smoking, the smoker releases `table->agent_sem`, or the agent
  never serves again.
- No deadlock, no matter which pairs come out in which order.

## Your task

Edit `smokers.hpp`. Implement `Smokers::start()`:

- spawn your worker threads (detached — they loop forever) and return
  immediately. From then on the class reacts to the agent: each round,
  the correct smoker calls `on_smoke(kind)` once (kind is `"tobacco"`,
  `"paper"`, or `"match"`), then releases `table_->agent_sem`.

Do not modify `AgentTable`, and never acquire `agent_sem` yourself —
that's the agent's side of the protocol.

## Traps worth savoring

The natural answer — each smoker waits directly on the two ingredient
semaphores it needs — is the famous wrong answer. The agent puts out
tobacco and paper; the match owner wakes on tobacco… but so can the
*paper* owner, who also waits on tobacco first. Now one token sits in the
wrong smoker's hand, the right smoker holds the other, and both block
forever on their second wait. The tests detect the wedge with bounded
waits and fail (rather than hang) — but they cannot tell you *which*
token went astray. Semaphores alone can't ask "are both available?"
atomically; something has to look at the whole table at once.

## Modern C++ notes (many ways to skin this cat)

- **Detached threads that own their state**: `start()` returns while its
  threads live forever, so nothing they touch may sit on the stack or in
  a member of a dying object. The pattern: put all shared state in one
  heap `State` struct, hold it via `std::shared_ptr`, and have every
  thread capture the `shared_ptr` *by value*. Destruction then follows
  the last thread, not the object that spawned it.
- This problem is **demultiplexing**: several event sources (ingredient
  semaphores), one decision that needs a coherent view of all of them.
  That is the same shape as a `select()`/`epoll` loop, and the classic
  solution — helper threads funneling events into a mutex-guarded
  scoreboard — is a hand-rolled MPSC (multi-producer single-consumer)
  dispatcher. In production C++ you'd reach for a small message queue
  and one dispatcher thread; the pusher pattern is exactly that
  dispatcher, unbundled.
- `std::counting_semaphore::try_acquire_for` is how the *tests* keep a
  wedged solution from hanging the suite — bounded waiting as a testing
  discipline, worth stealing for any concurrent code you verify.
- The field is `match_` not because C++ reserves `match` (it doesn't) but
  because shadow-prone names (`match`, `min`, `y1`…) cost more than the
  underscore does.

Run: `./check cpp 16`
