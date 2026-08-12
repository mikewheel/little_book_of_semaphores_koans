import time

from koan_utils import EventLog, ThreadRunner, assert_blocks, jitter

from rendezvous import Rendezvous


def run_pair(a_delay=0.0, b_delay=0.0):
    rv = Rendezvous()
    log = EventLog()
    runner = ThreadRunner()

    def a_body():
        if a_delay:
            time.sleep(a_delay)
        jitter()
        rv.run_a(lambda: log.record("a1"), lambda: log.record("a2"))

    def b_body():
        if b_delay:
            time.sleep(b_delay)
        jitter()
        rv.run_b(lambda: log.record("b1"), lambda: log.record("b2"))

    runner.spawn(a_body, name="A")
    runner.spawn(b_body, name="B")
    runner.join_all(timeout=5)
    return log


def assert_rendezvous_order(log):
    log.assert_before("a1", "b2")
    log.assert_before("b1", "a2")


def test_meeting_when_a_is_slow():
    for _ in range(10):
        assert_rendezvous_order(run_pair(a_delay=0.005))


def test_meeting_when_b_is_slow():
    for _ in range(10):
        assert_rendezvous_order(run_pair(b_delay=0.005))


def test_first_arrival_blocks_until_partner_shows_up():
    import threading

    rv = Rendezvous()
    log = EventLog()
    probe = assert_blocks(
        lambda: rv.run_a(lambda: log.record("a1"), lambda: log.record("a2")),
        msg="A must block at the rendezvous while B has not arrived",
    )
    threading.Thread(
        target=lambda: rv.run_b(lambda: log.record("b1"), lambda: log.record("b2")),
        daemon=True,
    ).start()
    assert probe.wait(5), "A should proceed once B arrives (deadlock?)"
    log.wait_for_count("b2", 1, timeout=5)
    assert_rendezvous_order(log)


def test_symmetric_first_arrival_b_blocks_too():
    import threading

    rv = Rendezvous()
    log = EventLog()
    probe = assert_blocks(
        lambda: rv.run_b(lambda: log.record("b1"), lambda: log.record("b2")),
        msg="B must block at the rendezvous while A has not arrived",
    )
    threading.Thread(
        target=lambda: rv.run_a(lambda: log.record("a1"), lambda: log.record("a2")),
        daemon=True,
    ).start()
    assert probe.wait(5), "B should proceed once A arrives (deadlock?)"
    log.wait_for_count("a2", 1, timeout=5)
    assert_rendezvous_order(log)


def test_does_not_overconstrain_a1_b1_order():
    """Either thread may finish its first statement first.

    A solution that serializes a1 before b1 (or vice versa) enforces too
    much. Over many jittered runs we must observe both orders.
    """
    orders = set()
    for _ in range(200):
        log = run_pair()
        events = log.events()
        first = "a1" if events.index("a1") < events.index("b1") else "b1"
        orders.add(first)
        assert_rendezvous_order(log)
        if len(orders) == 2:
            break
    assert orders == {"a1", "b1"}, (
        "over 200 runs only one of a1/b1 ever came first: "
        f"{orders} — the rendezvous is over-constrained"
    )


def test_stress():
    for _ in range(100):
        assert_rendezvous_order(run_pair())
