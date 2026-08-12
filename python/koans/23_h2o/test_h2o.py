import random
import threading
import time

from koan_utils import (
    EventLog,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from h2o import H2OBarrier


class BondHooks:
    """Records every bond; dawdles inside bond() to widen race windows."""

    def __init__(self):
        self.log = EventLog()

    def bond(self, kind):
        jitter(2.0)  # a slow chemistry set exposes molecules that smear
        self.log.record(kind)

    def bonds(self):
        return self.log.events()


def assert_molecules_well_formed(bonds):
    assert len(bonds) % 3 == 0, (
        f"bond count {len(bonds)} is not a multiple of 3: {bonds}"
    )
    for i in range(0, len(bonds), 3):
        triple = bonds[i : i + 3]
        assert sorted(triple) == ["H", "H", "O"], (
            f"molecule #{i // 3} is malformed: {triple} "
            f"(full bond sequence: {bonds})"
        )


def run_batch(n_h, n_o, join_timeout=15):
    hooks = BondHooks()
    barrier = H2OBarrier(hooks)
    runner = ThreadRunner()
    kinds = ["H"] * n_h + ["O"] * n_o
    random.shuffle(kinds)
    for i, kind in enumerate(kinds):
        def body(kind=kind):
            jitter()
            (barrier.hydrogen if kind == "H" else barrier.oxygen)()

        runner.spawn(body, name=f"atom-{kind}-{i}")
    runner.join_all(timeout=join_timeout)
    return hooks.bonds()


def test_lone_hydrogen_blocks():
    barrier = H2OBarrier(BondHooks())
    assert_blocks(
        barrier.hydrogen, msg="a lone hydrogen must wait for a full molecule"
    )


def test_two_hydrogens_block():
    hooks = BondHooks()
    barrier = H2OBarrier(hooks)
    assert_blocks(barrier.hydrogen, msg="one hydrogen alone must wait")
    assert_blocks(
        barrier.hydrogen,
        msg="two hydrogens without an oxygen must both keep waiting",
    )
    assert hooks.bonds() == [], "nothing may bond before a molecule is complete"


def test_lone_oxygen_blocks():
    barrier = H2OBarrier(BondHooks())
    assert_blocks(
        barrier.oxygen, msg="a lone oxygen must wait for two hydrogens"
    )


def test_h_h_o_completes():
    hooks = BondHooks()
    barrier = H2OBarrier(hooks)
    p1 = assert_blocks(barrier.hydrogen, msg="hydrogen #1 must wait")
    p2 = assert_blocks(barrier.hydrogen, msg="hydrogen #2 must wait")
    assert_completes(barrier.oxygen, timeout=5, msg="the completing oxygen")
    assert p1.wait(5), "hydrogen #1 should return once the molecule is complete"
    assert p2.wait(5), "hydrogen #2 should return once the molecule is complete"
    assert sorted(hooks.bonds()) == ["H", "H", "O"]


def test_molecules_are_well_formed():
    bonds = run_batch(20, 10)
    assert len(bonds) == 30, f"expected 30 bonds, saw {len(bonds)}"
    assert_molecules_well_formed(bonds)


def test_no_partial_molecule_left():
    hooks = BondHooks()
    barrier = H2OBarrier(hooks)
    done = EventLog()

    def atom(kind):
        try:
            (barrier.hydrogen if kind == "H" else barrier.oxygen)()
            done.record(kind)
        except BaseException as exc:  # surfaces in the eventually() message
            done.record(f"error: {exc!r}")

    for _ in range(5):
        threading.Thread(target=atom, args=("H",), daemon=True).start()
    for _ in range(2):
        threading.Thread(target=atom, args=("O",), daemon=True).start()

    eventually(
        lambda: len(done.events()) >= 6,
        timeout=5,
        msg="5 H + 2 O should yield two complete molecules",
    )
    time.sleep(0.3)  # give the stranded hydrogen every chance to leak through
    assert len(hooks.bonds()) == 6, (
        f"exactly 6 bonds expected (2 molecules), saw {hooks.bonds()}"
    )
    assert len(done.events()) == 6 and done.count("H") == 4 and done.count("O") == 2, (
        f"exactly 4 H and 2 O should have returned, saw {done.events()}"
    )
    # One more H and one more O rescue the stranded hydrogen.
    threading.Thread(target=atom, args=("H",), daemon=True).start()
    threading.Thread(target=atom, args=("O",), daemon=True).start()
    eventually(
        lambda: len(done.events()) == 9,
        timeout=5,
        msg="the stranded hydrogen never got its molecule",
    )
    assert_molecules_well_formed(hooks.bonds())


def test_stress():
    for _ in range(3):
        bonds = run_batch(40, 20, join_timeout=20)
        assert len(bonds) == 60
        assert_molecules_well_formed(bonds)
