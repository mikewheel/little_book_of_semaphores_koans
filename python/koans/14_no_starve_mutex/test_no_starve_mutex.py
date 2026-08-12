import inspect
import time

from koan_utils import (
    ThreadRunner,
    EventLog,
    assert_blocks,
    assert_completes,
    jitter,
)

from no_starve_mutex import NoStarveMutex

FORBIDDEN = (
    "threading.Semaphore",
    "threading.BoundedSemaphore",
    "threading.Lock",
    "threading.RLock",
    "threading.Condition",
    "threading.Event",
    "threading.Barrier",
)


def assert_honor_rule():
    src = inspect.getsource(NoStarveMutex)
    for token in FORBIDDEN:
        assert token not in src, (
            f"honor rule: {token} found inside NoStarveMutex — build it from "
            "WeakSemaphore instances and plain ints only"
        )


def test_honor_rule_and_basic_acquire_release():
    assert_honor_rule()
    m = NoStarveMutex()
    assert_completes(m.acquire, timeout=2, msg="an uncontended acquire must not block")
    m.release()
    assert_completes(m.acquire, timeout=2, msg="re-acquire after release must succeed")
    m.release()


def test_mutual_exclusion():
    """A racy read-modify-write must never lose updates under this lock."""
    assert_honor_rule()
    m = NoStarveMutex()
    state = {"value": 0}
    n_threads, n_iters = 4, 50
    runner = ThreadRunner()

    def worker():
        for i in range(n_iters):
            m.acquire()
            temp = state["value"]
            if i % 10 == 0:
                time.sleep(0.0005)  # invite the scheduler to interleave
            state["value"] = temp + 1
            m.release()

    for _ in range(n_threads):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert state["value"] == n_threads * n_iters, (
        f"lost updates: expected {n_threads * n_iters}, got {state['value']} — "
        "the critical section is not exclusive"
    )


def test_second_acquire_blocks_until_release():
    m = NoStarveMutex()
    assert_completes(m.acquire, timeout=2, msg="first acquire must not block")
    probe = assert_blocks(
        m.acquire, msg="second acquire must block while the lock is held"
    )
    m.release()
    assert probe.wait(5), "a waiter should get the lock after release"
    m.release()


N_THREADS = 8
LAPS = 150
# Morris bounds overtaking at roughly two "waiting rooms" worth of threads;
# the slack absorbs scheduling noise around the moment intent is recorded.
BOUND = 2 * N_THREADS + 4


def _max_overtakes(events, marked):
    """Worst number of foreign lock grants between the marked thread's
    declared intent (want) and its own grant (got)."""
    want, got = f"want:{marked}", f"got:{marked}"
    worst = count = 0
    in_window = False
    for e in events:
        if e == want:
            in_window, count = True, 0
        elif e == got:
            if in_window:
                worst = max(worst, count)
            in_window = False
        elif in_window and e.startswith("got:"):
            count += 1
    return worst


def test_bounded_overtaking():
    """THE test: with only weak semaphores underneath, nobody may be
    overtaken more than a bounded number of times per acquisition."""
    m = NoStarveMutex()
    log = EventLog()
    runner = ThreadRunner()

    def worker(i):
        for _ in range(LAPS):
            log.record(f"want:{i}")
            m.acquire()
            log.record(f"got:{i}")
            time.sleep(0.0001)  # a sliver of critical-section work
            m.release()
            jitter(0.3)

    for i in range(N_THREADS):
        runner.spawn(worker, i, name=f"P{i}")
    runner.join_all(timeout=45)

    events = log.events()
    assert log.count("got:0") == LAPS
    worst = _max_overtakes(events, marked=0)
    assert worst <= BOUND, (
        f"thread 0 was overtaken {worst} times while waiting for one "
        f"acquisition (bound: {BOUND}) — a weak semaphore is starving it"
    )


def test_progress_under_contention():
    """Heavy contention must not deadlock; every thread finishes its laps."""
    m = NoStarveMutex()
    state = {"count": 0}
    n_threads, laps = 8, 60
    runner = ThreadRunner()

    def worker():
        for _ in range(laps):
            m.acquire()
            state["count"] += 1  # protected by the lock under test
            m.release()
            jitter(0.3)

    for _ in range(n_threads):
        runner.spawn(worker)
    runner.join_all(timeout=30)
    assert state["count"] == n_threads * laps
