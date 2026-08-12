import random
import threading
import time

from koan_utils import (
    EventLog,
    ThreadRunner,
    eventually,
    jitter,
)

from river import Boat


class BoatHooks:
    """Records boards and rows; dawdles inside board() to widen windows."""

    def __init__(self):
        self.log = EventLog()

    def board(self, kind):
        jitter(2.0)
        self.log.record(f"board:{kind}")

    def row_boat(self, kind):
        self.log.record("row")

    def boards(self):
        return [e.split(":", 1)[1] for e in self.log.events() if e.startswith("board:")]

    def rows(self):
        return self.log.count("row")


def assert_boatloads_legal(boards):
    assert len(boards) % 4 == 0, (
        f"board count {len(boards)} is not a multiple of 4: {boards}"
    )
    for i in range(0, len(boards), 4):
        block = boards[i : i + 4]
        h, s = block.count("hacker"), block.count("serf")
        assert (h, s) in ((4, 0), (0, 4), (2, 2)), (
            f"boatload #{i // 4} is illegal: {block} (full sequence: {boards})"
        )


def assert_full_pattern(events):
    """Each boatload: exactly 4 boards, then exactly one row, no overlap."""
    pending = 0
    for e in events:
        if e.startswith("board:"):
            pending += 1
            assert pending <= 4, (
                f"a fifth passenger boarded before the boat sailed: {events}"
            )
        elif e == "row":
            assert pending == 4, (
                f"row_boat fired before all four passengers boarded: {events}"
            )
            pending = 0
    assert pending == 0, f"a boatload boarded but never sailed: {events}"


def run_mix(n_hackers, n_serfs, join_timeout=15):
    hooks = BoatHooks()
    boat = Boat(hooks)
    runner = ThreadRunner()
    kinds = ["hacker"] * n_hackers + ["serf"] * n_serfs
    random.shuffle(kinds)
    for i, kind in enumerate(kinds):
        def body(kind=kind):
            jitter()
            (boat.hacker_arrives if kind == "hacker" else boat.serf_arrives)()

        runner.spawn(body, name=f"{kind}-{i}")
    runner.join_all(timeout=join_timeout)
    return hooks


def test_three_and_one_never_sails():
    hooks = BoatHooks()
    boat = Boat(hooks)
    done = EventLog()

    def rider(kind):
        try:
            (boat.hacker_arrives if kind == "hacker" else boat.serf_arrives)()
            done.record(kind)
        except BaseException as exc:  # surfaced by the asserts below
            done.record(f"error: {exc!r}")

    for _ in range(3):
        threading.Thread(target=rider, args=("hacker",), daemon=True).start()
    threading.Thread(target=rider, args=("serf",), daemon=True).start()
    time.sleep(0.4)  # 3 hackers + 1 serf: give an illegal crew every chance
    assert hooks.log.events() == [], (
        f"3 hackers + 1 serf must not board anything, saw {hooks.log.events()}"
    )
    assert done.events() == [], (
        f"nobody may cross in an illegal combination: {done.events()}"
    )
    # A fourth hacker makes an all-hacker crew possible; the serf stays.
    threading.Thread(target=rider, args=("hacker",), daemon=True).start()
    eventually(
        lambda: done.count("hacker") == 4,
        timeout=5,
        msg="four hackers should sail together once the fourth arrives",
    )
    assert hooks.boards() == ["hacker"] * 4, (
        f"expected exactly the 4 hackers to board, saw {hooks.boards()}"
    )
    assert hooks.rows() == 1
    time.sleep(0.3)
    assert done.count("serf") == 0, "the lone serf must keep waiting ashore"


def test_pairs_combination_sails():
    hooks = run_mix(2, 2, join_timeout=5)
    assert sorted(hooks.boards()) == ["hacker", "hacker", "serf", "serf"]
    assert hooks.rows() == 1


def test_four_of_a_kind_sails():
    hooks = run_mix(0, 4, join_timeout=5)
    assert hooks.boards() == ["serf"] * 4
    assert hooks.rows() == 1


def test_rowing_after_all_board():
    for n_h, n_s in ((2, 2), (4, 0), (0, 4)):
        hooks = run_mix(n_h, n_s, join_timeout=5)
        events = hooks.log.events()
        assert hooks.rows() == 1, f"exactly one rower per boatload: {events}"
        row_index = events.index("row")
        board_indexes = [i for i, e in enumerate(events) if e.startswith("board:")]
        assert len(board_indexes) == 4
        assert all(i < row_index for i in board_indexes), (
            f"row_boat must come after all four boards: {events}"
        )


def test_boatloads_do_not_interleave():
    hooks = run_mix(8, 8)
    assert_boatloads_legal(hooks.boards())
    assert hooks.rows() == 4
    assert_full_pattern(hooks.log.events())


def test_stress():
    hooks = run_mix(16, 16, join_timeout=20)
    assert len(hooks.boards()) == 32
    assert_boatloads_legal(hooks.boards())
    assert hooks.rows() == 8
    assert_full_pattern(hooks.log.events())
