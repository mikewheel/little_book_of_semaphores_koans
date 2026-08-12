import time

from koan_utils import (
    EventLog,
    ThreadRunner,
    assert_blocks,
    assert_completes,
    jitter,
)

from signaling import Signaling


def test_b1_runs_after_a1_even_when_b_starts_first():
    for _ in range(30):
        sig = Signaling()
        log = EventLog()
        runner = ThreadRunner()
        # B gets a head start; A dawdles before running a1.
        runner.spawn(lambda: sig.run_b(lambda: log.record("b1")), name="B")

        def a_body():
            time.sleep(0.002)
            sig.run_a(lambda: log.record("a1"))

        runner.spawn(a_body, name="A")
        runner.join_all(timeout=5)
        assert log.events() == ["a1", "b1"], (
            f"b1 must come after a1; log was {log.events()}"
        )


def test_b_blocks_until_a_signals():
    sig = Signaling()
    log = EventLog()
    probe = assert_blocks(
        lambda: sig.run_b(lambda: log.record("b1")),
        msg="run_b must block while A has not yet run",
    )
    sig.run_a(lambda: log.record("a1"))
    assert probe.wait(5), "run_b should have been released by A's signal"
    log.assert_before("a1", "b1")


def test_a_never_waits_for_b():
    sig = Signaling()
    assert_completes(
        lambda: sig.run_a(lambda: None),
        timeout=2,
        msg="run_a must not block even if B never shows up",
    )


def test_signal_persists_if_a_finishes_first():
    sig = Signaling()
    log = EventLog()
    sig.run_a(lambda: log.record("a1"))
    time.sleep(0.05)  # the signal must not evaporate
    assert_completes(lambda: sig.run_b(lambda: log.record("b1")), timeout=2)
    assert log.events() == ["a1", "b1"]


def test_stress_random_interleavings():
    for _ in range(100):
        sig = Signaling()
        log = EventLog()
        runner = ThreadRunner()

        def b_body(sig=sig, log=log):
            jitter()
            sig.run_b(lambda: log.record("b1"))

        def a_body(sig=sig, log=log):
            jitter()
            sig.run_a(lambda: log.record("a1"))

        runner.spawn(b_body, name="B")
        runner.spawn(a_body, name="A")
        runner.join_all(timeout=5)
        assert log.events() == ["a1", "b1"], f"log was {log.events()}"
