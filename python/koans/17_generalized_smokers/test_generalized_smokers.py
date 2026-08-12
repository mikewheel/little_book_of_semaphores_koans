import random
import time

from koan_utils import EventLog, eventually, jitter

from generalized_smokers import AgentTable, GeneralizedSmokers, INGREDIENTS

# Each pair the agent can put out, and who must smoke it (the complement).
COMPLEMENT = {
    ("tobacco", "paper"): "match",
    ("paper", "match"): "tobacco",
    ("tobacco", "match"): "paper",
}
PAIRS = list(COMPLEMENT)


def start_smokers():
    table = AgentTable()
    log = EventLog()
    GeneralizedSmokers(table, log.record).start()
    return table, log


def blast_pairs(table, rounds, pause_ms):
    """Fire `rounds` random pairs; returns how often each owner must smoke."""
    owed = {kind: 0 for kind in INGREDIENTS}
    for _ in range(rounds):
        pair = random.choice(PAIRS)
        owed[COMPLEMENT[pair]] += 1
        for kind in pair:
            table.ingredient_sem(kind).release()
        if pause_ms:
            jitter(pause_ms)
    return owed


def assert_all_smoked(log, owed, rounds):
    try:
        eventually(lambda: len(log.events()) >= rounds, timeout=10)
    except AssertionError:
        raise AssertionError(
            f"only {len(log.events())} of {rounds} cigarettes were ever "
            "smoked — ingredients got lost (noted down, or overwritten?)"
        ) from None
    time.sleep(0.25)  # any over-smoking shows up now
    assert len(log.events()) == rounds, (
        f"expected exactly {rounds} cigarettes, saw {len(log.events())}: "
        f"{log.events()}"
    )
    for kind in INGREDIENTS:
        assert log.count(kind) == owed[kind], (
            f"conservation violated for {kind!r}: its owner smoked "
            f"{log.count(kind)} times but the released pairs entitled it to "
            f"{owed[kind]} — an ingredient was lost or double-counted"
        )


def test_no_smoke_without_ingredients():
    table, log = start_smokers()
    time.sleep(0.3)  # every chance to misbehave
    assert log.events() == [], (
        f"smokers smoked {log.events()} before any ingredients existed"
    )


def test_all_cigarettes_eventually_smoked():
    table, log = start_smokers()
    rounds = 60
    owed = blast_pairs(table, rounds, pause_ms=1.0)
    assert_all_smoked(log, owed, rounds)


def test_per_smoker_counts_match_conservation():
    """Fixed diet: released ingredient counts pin down every smoke tally."""
    table, log = start_smokers()
    schedule = (
        [("tobacco", "paper")] * 10
        + [("paper", "match")] * 8
        + [("tobacco", "match")] * 6
    )
    random.shuffle(schedule)
    for pair in schedule:
        for kind in pair:
            table.ingredient_sem(kind).release()
        jitter(0.5)
    eventually(lambda: len(log.events()) >= 24, timeout=10)
    time.sleep(0.25)
    assert log.count("match") == 10   # tobacco+paper rounds
    assert log.count("tobacco") == 8  # paper+match rounds
    assert log.count("paper") == 6    # tobacco+match rounds


def test_burst_stress():
    """Twenty copies of one pair dumped at once (then the next pair type):
    many duplicate tokens are pending together, and every one must be
    remembered. (Bursts of a single pair type keep the bookkeeping
    schedule-independent; a boolean scoreboard still drowns.)"""
    table, log = start_smokers()
    per_type = 20
    smoked = 0
    for pair in PAIRS:
        for _ in range(per_type):
            for kind in pair:
                table.ingredient_sem(kind).release()
        smoked += per_type
        expect = smoked
        try:
            eventually(lambda: len(log.events()) >= expect, timeout=10)
        except AssertionError:
            raise AssertionError(
                f"a burst of {per_type} x {pair} yielded only "
                f"{len(log.events()) - (expect - per_type)} cigarettes — "
                "duplicate ingredients were forgotten"
            ) from None
    time.sleep(0.25)  # any over-smoking shows up now
    assert len(log.events()) == smoked
    for pair in PAIRS:
        assert log.count(COMPLEMENT[pair]) == per_type, (
            f"{COMPLEMENT[pair]!r}'s owner should have smoked {per_type} "
            f"(one per {pair} released), got {log.count(COMPLEMENT[pair])}"
        )
