# Koan 17 — Generalized smokers

*Adapted from* The Little Book of Semaphores, *§4.5 (CC BY-NC-SA 4.0).*

## The problem

Parnas's twist on koan 16: fire the agent's pacing. The agent no longer
waits for a cigarette before putting out the next pair of ingredients —
it releases pair after pair as fast as it likes. Consequently the table
can hold *several tokens of the same ingredient* at once: two tobaccos
and a paper, five matches, whatever the burst brings.

The smokers' side of the contract tightens into a conservation law. If
the agent releases pairs `M` times in total, and `T` of those pairs
contained tobacco, then:

- exactly `M` cigarettes get smoked, eventually;
- the tobacco owner smokes exactly `M − T` of them (it smokes precisely
  when the pair was paper+match), and likewise for the other two;
- nothing is smoked before its ingredients exist, and nothing is smoked
  twice.

In this koan the tests play the agent, using the provided `AgentTable`
(same shape as koan 16, but `agent_sem` starts at 0 and is never touched
by anyone — there is no turn-taking left to signal).

## Your task

Edit `generalized_smokers.py`. Implement `GeneralizedSmokers(table,
on_smoke)`:

- `start()` — spawn your worker threads (daemons — they loop forever) and
  return immediately. Whenever a smoker rolls and smokes, call
  `on_smoke(kind)` with that smoker's *own* ingredient. Do not touch
  `agent_sem`.

Do not modify `AgentTable`.

## Traps worth savoring

Port the koan 16 answer verbatim and it will *usually* work — then a
burst puts two tobaccos on the table before any bookkeeping runs, and a
"tobacco is present" flag quietly swallows the second one. A boolean can
say only *whether*; this agent requires *how many*. The conservation
tests count every token, so anything lost to an overwrite shows up as a
final tally mismatch.

## Python notes

The moral is about state granularity, not smoking: when producers stop
taking turns, presence flags must graduate to counters (and eventually to
queues). The compound check "two counters positive → decrement both and
dispatch" still has to be atomic, which is why the mutex survives even
though the booleans didn't.

Run: `./check python 17`
