import threading
import time

from koan_utils import EventLog, ThreadRunner, eventually, jitter

from santa import NorthPole


class Hooks:
    """Records every hook call; get_help can be gated by a test event."""

    def __init__(self, get_help_gate=None):
        self.log = EventLog()
        self._gate = get_help_gate

    def prepare_sleigh(self):
        self.log.record("sleigh")

    def get_hitched(self, rid):
        self.log.record(f"hitched{rid}")

    def help_elves(self):
        self.log.record("help")

    def get_help(self, eid):
        if self._gate is not None:
            assert self._gate.wait(10), "test gate never released"
        self.log.record(f"helped{eid}")


def test_reindeer_flight():
    hooks = Hooks()
    pole = NorthPole(hooks)
    pole.start_santa()
    runner = ThreadRunner()

    # Eight reindeer home: Santa must keep sleeping — no sleigh yet.
    for rid in range(8):
        runner.spawn(pole.reindeer_arrives, rid, name=f"reindeer-{rid}")
        jitter()
    time.sleep(0.3)
    runner.raise_worker_errors()
    assert hooks.log.count("sleigh") == 0, (
        "Santa prepped the sleigh before the last reindeer was home"
    )

    # The ninth springs him into action: one sleigh, nine hitchings.
    runner.spawn(pole.reindeer_arrives, 8, name="reindeer-8")
    runner.join_all(timeout=10)
    assert hooks.log.count("sleigh") == 1
    hitched = [e for e in hooks.log.events() if e.startswith("hitched")]
    assert sorted(hitched) == sorted(f"hitched{r}" for r in range(9))
    hooks.log.assert_before("sleigh", "hitched0")  # prep first, then hitch


def test_elves_in_batches_of_three():
    hooks = Hooks()
    pole = NorthPole(hooks)
    pole.start_santa()
    runner = ThreadRunner()
    for eid in range(3):
        runner.spawn(pole.elf_needs_help, eid, name=f"elf-{eid}")
        jitter()
    runner.join_all(timeout=10)
    assert hooks.log.count("help") == 1, "one help_elves per group of 3"
    helped = [e for e in hooks.log.events() if e.startswith("helped")]
    assert sorted(helped) == ["helped0", "helped1", "helped2"]
    hooks.log.assert_before("help", "helped0")  # Santa helps, then they get it


def test_fourth_elf_waits_for_a_new_group():
    gate = threading.Event()
    hooks = Hooks(get_help_gate=gate)
    pole = NorthPole(hooks)
    pole.start_santa()
    runner = ThreadRunner()

    # A full group of three walks in; their get_help hangs on the gate.
    for eid in range(3):
        runner.spawn(pole.elf_needs_help, eid, name=f"elf-{eid}")
    eventually(
        lambda: runner.raise_worker_errors() or hooks.log.count("help") == 1,
        timeout=5,
        msg="Santa never helped the first group",
    )

    # Three more elves arrive mid-help: none may sneak into the group in
    # progress — they must wait at the door and form the NEXT group.
    for eid in range(3, 6):
        runner.spawn(pole.elf_needs_help, eid, name=f"elf-{eid}")
    time.sleep(0.3)
    runner.raise_worker_errors()
    for eid in range(3, 6):
        assert hooks.log.count(f"helped{eid}") == 0, (
            "an elf joined a group that was already being helped"
        )
    assert hooks.log.count("help") == 1

    gate.set()  # first group finishes; the second forms and gets helped
    runner.join_all(timeout=10)
    assert hooks.log.count("help") == 2
    for eid in range(6):
        assert hooks.log.count(f"helped{eid}") == 1

    # And a lone straggler keeps waiting until two buddies show up.
    runner.spawn(pole.elf_needs_help, 6, name="elf-6")
    time.sleep(0.3)
    runner.raise_worker_errors()
    assert hooks.log.count("helped6") == 0, (
        "an elf was helped without a full group"
    )
    runner.spawn(pole.elf_needs_help, 7, name="elf-7")
    runner.spawn(pole.elf_needs_help, 8, name="elf-8")
    runner.join_all(timeout=10)
    assert hooks.log.count("help") == 3
    assert hooks.log.count("helped6") == 1


def test_multiple_cycles():
    hooks = Hooks()
    pole = NorthPole(hooks)
    pole.start_santa()
    runner = ThreadRunner()
    for eid in range(12):  # four full elf groups
        runner.spawn(pole.elf_needs_help, eid, name=f"elf-{eid}")
        jitter()
    for rid in range(9):  # flight one
        runner.spawn(pole.reindeer_arrives, rid, name=f"reindeer-{rid}")
        jitter()
    # The same nine reindeer fly every year: the next herd cannot start
    # arriving until this year's flight is fully hitched (see README).
    eventually(
        lambda: runner.raise_worker_errors()
        or sum(1 for e in hooks.log.events() if e.startswith("hitched")) == 9,
        timeout=10,
        msg="the first flight never finished",
    )
    for rid in range(9, 18):  # flight two
        runner.spawn(pole.reindeer_arrives, rid, name=f"reindeer-{rid}")
        jitter()
    runner.join_all(timeout=15)

    assert hooks.log.count("sleigh") == 2, "18 reindeer == exactly 2 flights"
    assert hooks.log.count("help") == 4, "12 elves == exactly 4 groups"
    assert sum(1 for e in hooks.log.events() if e.startswith("hitched")) == 18
    assert sum(1 for e in hooks.log.events() if e.startswith("helped")) == 12


def test_batch_atomicity_stress():
    # Nine elves at once — spawned as fast as possible so arrivals overlap
    # groups already in flight: exactly three groups, and the three elves
    # helped after each help_elves must be three distinct elves who never
    # appear in a later group.
    hooks = Hooks()
    pole = NorthPole(hooks)
    pole.start_santa()
    runner = ThreadRunner()
    for eid in range(9):
        runner.spawn(pole.elf_needs_help, eid, name=f"elf-{eid}")
    runner.join_all(timeout=15)

    events = hooks.log.events()
    assert hooks.log.count("help") == 3, f"9 elves == exactly 3 groups: {events}"

    groups, current = [], None
    for e in events:
        if e == "help":
            current = set()
            groups.append(current)
        elif e.startswith("helped"):
            assert current is not None, f"help arrived after get_help: {events}"
            current.add(e[6:])
    assert [len(g) for g in groups] == [3, 3, 3], (
        f"each group must be exactly 3 elves: {events}"
    )
    seen = set()
    for g in groups:
        assert not (g & seen), f"an elf appears in two groups: {events}"
        seen |= g
    assert seen == {str(e) for e in range(9)}
