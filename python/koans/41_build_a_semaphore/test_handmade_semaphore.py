import inspect
import re
import threading
import time

import handmade_semaphore
from koan_utils import (
    EventLog,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from handmade_semaphore import HandmadeSemaphore


def test_initial_value_admits_that_many():
    sem = HandmadeSemaphore(2)
    assert_completes(sem.acquire, timeout=2)
    assert_completes(sem.acquire, timeout=2)
    probe = assert_blocks(sem.acquire, msg="the third acquire must block")
    sem.release()
    assert probe.wait(5), "a release should have unblocked the third acquire"


def test_zero_starts_blocked():
    sem = HandmadeSemaphore(0)
    probe = assert_blocks(sem.acquire, msg="acquire on a zero semaphore must block")
    sem.release()
    assert probe.wait(5), "a release should have unblocked the acquire"


def test_release_before_acquire_banks_a_token():
    sem = HandmadeSemaphore(0)
    sem.release()  # nobody is waiting: the permit must be banked, not lost
    sem.release()
    assert_completes(sem.acquire, timeout=2)
    assert_completes(sem.acquire, timeout=2)
    probe = assert_blocks(sem.acquire, msg="the bank should now be empty")
    sem.release()
    assert probe.wait(5)


def test_wakes_exactly_one():
    sem = HandmadeSemaphore(0)
    log = EventLog()
    runner = ThreadRunner()

    def waiter(i):
        sem.acquire()
        log.record("done")

    for i in range(4):
        runner.spawn(waiter, i, name=f"w{i}")
    time.sleep(0.3)  # let all four park
    runner.raise_worker_errors()
    assert log.count("done") == 0
    sem.release()
    eventually(lambda: log.count("done") == 1, timeout=5)
    time.sleep(0.3)  # nobody else should sneak through
    runner.raise_worker_errors()
    assert log.count("done") == 1, (
        f"one release woke {log.count('done')} waiters — exactly one must get in"
    )
    for _ in range(3):
        sem.release()
    runner.join_all(timeout=10)
    assert log.count("done") == 4


def test_released_token_is_reserved_for_a_waiter():
    """Property 3: a release must go to a parked waiter, and a fresh
    acquirer racing in right behind the release must queue up instead of
    snatching the wakeup (the classic stolen-signal bug)."""
    sem = HandmadeSemaphore(0)
    runner = ThreadRunner()
    for round_no in range(6):
        log = EventLog()
        runner.spawn(lambda: (sem.acquire(), log.record("got_it")), name=f"w{round_no}")
        time.sleep(0.3)  # let the waiter park
        runner.raise_worker_errors()
        # Release, then immediately re-acquire from this very thread: the
        # thief-shaped move. The permit is spoken for, so this must block.
        probe = assert_blocks(
            lambda: (sem.release(), sem.acquire()),
            msg="a release immediately followed by acquire stole the wakeup "
            "reserved for the parked waiter (round %d)" % round_no,
        )
        eventually(
            lambda: log.count("got_it") == 1,
            timeout=5,
            msg="the parked waiter never received the released permit "
            f"(round {round_no})",
        )
        sem.release()  # now free the would-be thief
        assert probe.wait(5)
    runner.join_all(timeout=10)


def test_no_lost_wakeups_stress():
    ping = HandmadeSemaphore(0)
    pong = HandmadeSemaphore(0)
    log = EventLog()
    runner = ThreadRunner()
    rounds = 50

    def pinger():
        for _ in range(rounds):
            ping.release()
            pong.acquire()
            jitter(1)
        log.record("ping_done")

    def ponger():
        for _ in range(rounds):
            ping.acquire()
            pong.release()
            jitter(1)
        log.record("pong_done")

    for i in range(4):
        runner.spawn(pinger, name=f"ping{i}")
        runner.spawn(ponger, name=f"pong{i}")
    runner.join_all(timeout=10)
    assert log.count("ping_done") == 4
    assert log.count("pong_done") == 4


def test_works_as_mutex():
    sem = HandmadeSemaphore(1)
    state = {"count": 0}
    runner = ThreadRunner()
    per_thread = 300

    def worker():
        for n in range(per_thread):
            sem.acquire()
            v = state["count"]
            if n % 97 == 0:
                time.sleep(0.0005)  # widen the race window
            state["count"] = v + 1
            sem.release()

    for i in range(8):
        runner.spawn(worker, name=f"m{i}")
    runner.join_all(timeout=10)
    assert state["count"] == 8 * per_thread, (
        f"lost updates: got {state['count']}, expected {8 * per_thread} — "
        "the semaphore does not provide mutual exclusion"
    )


def test_implementation_does_not_cheat():
    src = inspect.getsource(handmade_semaphore)
    banned = [
        r"threading\s*\.\s*(Bounded)?Semaphore",
        r"from\s+threading\s+import[^\n]*Semaphore",
    ]
    for pattern in banned:
        assert not re.search(pattern, src), (
            "handmade_semaphore.py must be built from Lock/Condition only; "
            f"found a use of the stdlib semaphore (pattern: {pattern})"
        )
    # ...and the machinery must actually work, not merely be honest.
    sem = HandmadeSemaphore(1)
    assert_completes(sem.acquire, timeout=2)
    sem.release()
