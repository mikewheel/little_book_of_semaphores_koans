import threading

from koan_utils import (
    OverlapTracker,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    eventually,
    jitter,
)

from list_guard import ListGuard


def test_searchers_share():
    guard = ListGuard()
    tracker = OverlapTracker()
    release = threading.Event()
    runner = ThreadRunner()

    def searcher():
        guard.search_enter()
        tracker.enter("search")
        release.wait(5)  # linger until everyone is inside together
        tracker.exit("search")
        guard.search_exit()

    for _ in range(4):
        runner.spawn(searcher)
    eventually(
        lambda: tracker.current("search") == 4,
        timeout=5,
        msg=f"only {tracker.current('search')} of 4 searchers made it inside "
        "concurrently — searchers must not exclude each other",
    )
    release.set()
    runner.join_all(timeout=5)


def test_inserter_with_searchers():
    guard = ListGuard()
    tracker = OverlapTracker()
    release = threading.Event()
    runner = ThreadRunner()

    def searcher():
        guard.search_enter()
        tracker.enter("search")
        release.wait(5)
        tracker.exit("search")
        guard.search_exit()

    runner.spawn(searcher)
    runner.spawn(searcher)
    eventually(lambda: tracker.current("search") == 2, timeout=5)

    def inserter():
        guard.insert_enter()
        snap = tracker.enter("insert")
        tracker.exit("insert")
        guard.insert_exit()
        return snap

    snap = assert_completes(
        inserter,
        timeout=5,
        msg="an inserter must be able to work while searchers are inside",
    )
    assert snap.get("search") == 2 and snap.get("insert") == 1, (
        f"expected the inserter to witness both searchers: {snap}"
    )
    release.set()
    runner.join_all(timeout=5)


def test_inserters_mutually_exclusive():
    guard = ListGuard()
    tracker = OverlapTracker()
    release = threading.Event()
    runner = ThreadRunner()

    def parked_searcher():  # stays inside for the whole test
        guard.search_enter()
        tracker.enter("search")
        release.wait(10)
        tracker.exit("search")
        guard.search_exit()

    runner.spawn(parked_searcher)
    eventually(lambda: tracker.current("search") == 1, timeout=5)

    inserters_done = []

    def inserter():
        for _ in range(20):
            guard.insert_enter()
            snap = tracker.enter("insert")
            if snap.get("insert", 0) > 1:
                tracker.violate(f"two inserters inside at once: {snap}")
            if snap.get("search", 0) < 1:
                tracker.violate(
                    f"the parked searcher vanished from the snapshot: {snap}"
                )
            jitter(1.0)
            tracker.exit("insert")
            guard.insert_exit()
            jitter()  # give the other inserter a turn
        inserters_done.append(True)

    runner.spawn(inserter)
    runner.spawn(inserter)
    eventually(
        lambda: len(inserters_done) == 2,
        timeout=15,
        msg="the inserters never finished their loops (blocked by whom?)",
    )
    release.set()  # only now may the parked searcher leave
    runner.join_all(timeout=5)
    tracker.assert_no_violations()
    assert tracker.max_concurrent("insert") == 1, (
        "two inserters overlapped: max concurrent inserters was "
        f"{tracker.max_concurrent('insert')}"
    )


def test_deleter_fully_exclusive():
    guard = ListGuard()
    tracker = OverlapTracker()
    snapshots = []

    def deleter():
        guard.delete_enter()
        snap = tracker.enter("delete")
        tracker.exit("delete")
        guard.delete_exit()
        snapshots.append(snap)

    # While a searcher is inside, the deleter must wait.
    guard.search_enter()
    probe = assert_blocks(
        deleter, msg="a deleter must wait while a searcher is inside"
    )
    guard.search_exit()
    assert probe.wait(5), "the deleter should proceed once the searcher leaves"
    snap = snapshots[-1]
    assert (
        snap.get("delete") == 1
        and snap.get("search", 0) == 0
        and snap.get("insert", 0) == 0
    ), f"the deleter must be alone at entry: {snap}"

    # While an inserter is inside, the deleter must wait.
    guard.insert_enter()
    probe = assert_blocks(
        deleter, msg="a deleter must wait while an inserter is inside"
    )
    guard.insert_exit()
    assert probe.wait(5), "the deleter should proceed once the inserter leaves"

    # While the deleter is inside, searchers and inserters must wait.
    guard.delete_enter()
    search_probe = assert_blocks(
        guard.search_enter, msg="a searcher must wait while a deleter is inside"
    )
    insert_probe = assert_blocks(
        guard.insert_enter, msg="an inserter must wait while a deleter is inside"
    )
    guard.delete_exit()
    assert search_probe.wait(5), "the searcher should enter once the deleter leaves"
    assert insert_probe.wait(5), "the inserter should enter once the deleter leaves"


def test_invariant_stress():
    guard = ListGuard()
    tracker = OverlapTracker()
    runner = ThreadRunner()

    def searcher():
        for _ in range(12):
            jitter()
            guard.search_enter()
            snap = tracker.enter("search")
            if snap.get("delete", 0):
                tracker.violate(f"searcher entered during a delete: {snap}")
            jitter(1.0)
            tracker.exit("search")
            guard.search_exit()

    def inserter():
        for _ in range(12):
            jitter()
            guard.insert_enter()
            snap = tracker.enter("insert")
            if snap.get("insert", 0) > 1:
                tracker.violate(f"two inserters inside at once: {snap}")
            if snap.get("delete", 0):
                tracker.violate(f"inserter entered during a delete: {snap}")
            jitter(1.0)
            tracker.exit("insert")
            guard.insert_exit()

    def deleter():
        for _ in range(12):
            jitter()
            guard.delete_enter()
            snap = tracker.enter("delete")
            if (
                snap.get("search", 0)
                or snap.get("insert", 0)
                or snap.get("delete") != 1
            ):
                tracker.violate(f"deleter was not alone: {snap}")
            jitter(1.0)
            tracker.exit("delete")
            guard.delete_exit()

    for _ in range(4):
        runner.spawn(searcher)
    for _ in range(2):
        runner.spawn(inserter)
    for _ in range(2):
        runner.spawn(deleter)
    runner.join_all(timeout=20)
    tracker.assert_no_violations()
    assert tracker.max_concurrent("insert") <= 1
