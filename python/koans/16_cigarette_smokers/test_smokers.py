import random
import time

from koan_utils import EventLog, eventually

from smokers import AgentTable, Smokers, INGREDIENTS

# Each pair of ingredients the agent can put out, and who must smoke.
COMPLEMENT = {
    ("tobacco", "paper"): "match",
    ("paper", "match"): "tobacco",
    ("tobacco", "match"): "paper",
}
PAIRS = list(COMPLEMENT)


def start_smokers():
    table = AgentTable()
    log = EventLog()
    smokers = Smokers(table, log.record)
    smokers.start()
    return table, log


def play_round(table, log, pair, served_before):
    """One agent turn: wait to be signaled, put out two ingredients, then
    wait (bounded!) for exactly one smoke of the right kind."""
    assert table.agent_sem.acquire(timeout=5), (
        "the agent never got the go-ahead — is agent_sem released after "
        "each smoke?"
    )
    for kind in pair:
        table.ingredient_sem(kind).release()
    eventually(
        lambda: len(log.events()) >= served_before + 1,
        timeout=5,
        msg=f"nobody smoked after the agent put out {pair} — deadlock? "
        "(smokers must not grab ingredients they cannot use)",
    )
    events = log.events()
    assert len(events) == served_before + 1, (
        f"expected exactly one smoke for {pair}, saw {events[served_before:]}"
    )
    assert events[-1] == COMPLEMENT[pair], (
        f"the agent put out {pair}; the smoker owning "
        f"{COMPLEMENT[pair]!r} had to smoke, but {events[-1]!r} did"
    )


def test_no_smoke_before_the_agent_acts():
    table, log = start_smokers()
    time.sleep(0.3)  # every chance to misbehave
    assert log.events() == [], (
        f"smokers smoked {log.events()} before any ingredients existed"
    )


def test_only_matching_smoker_smokes():
    """Cycle deterministically through all three pairs, 30 rounds."""
    table, log = start_smokers()
    for r in range(30):
        play_round(table, log, PAIRS[r % 3], served_before=r)


def test_no_spurious_smokes():
    table, log = start_smokers()
    rounds = 15
    for r in range(rounds):
        play_round(table, log, random.choice(PAIRS), served_before=r)
    # The final agent_sem signal must be there, and then: silence.
    assert table.agent_sem.acquire(timeout=5)
    time.sleep(0.25)
    assert len(log.events()) == rounds, (
        f"smoke count changed after the agent stopped: {log.events()}"
    )


def test_stress_random_pairs():
    table, log = start_smokers()
    tally = {kind: 0 for kind in INGREDIENTS}
    for r in range(60):
        pair = random.choice(PAIRS)
        tally[COMPLEMENT[pair]] += 1
        play_round(table, log, pair, served_before=r)
    for kind in INGREDIENTS:
        assert log.count(kind) == tally[kind], (
            f"the {kind}-owning smoker smoked {log.count(kind)} times; "
            f"the agent's pairs entitled it to {tally[kind]}"
        )
