import threading
import time
from collections import deque

from koan_utils import ThreadRunner, assert_blocks, assert_completes, jitter

from event_buffer import ProducerConsumer

# Sentinel returned by GuardedBuffer.get() on an empty buffer, instead of
# crashing the calling thread. An int so result multisets stay sortable.
EMPTY = -999_999


class GuardedBuffer:
    """An instrumented buffer that DETECTS concurrent access.

    add()/get() raise an "operation in flight" count on entry, dwell a
    moment to widen any race window, do the real work, then lower it. If
    two operations are ever in flight together, the user's solution failed
    to serialize buffer access, and we record a violation. get() on an
    empty buffer records a violation and returns the EMPTY sentinel.

    (The internal lock only protects the instrumentation and the deque
    from corruption — it deliberately does NOT serialize the dwell, so a
    non-exclusive solution still gets caught.)
    """

    def __init__(self):
        self._items = deque()
        self._lock = threading.Lock()
        self._in_flight = 0
        self.violations = []

    def _begin(self, op):
        with self._lock:
            self._in_flight += 1
            if self._in_flight > 1:
                self.violations.append(
                    f"{op}() overlapped another buffer operation"
                )

    def _end(self):
        with self._lock:
            self._in_flight -= 1

    def add(self, item):
        self._begin("add")
        time.sleep(0.0002)
        with self._lock:
            self._items.append(item)
        self._end()

    def get(self):
        self._begin("get")
        time.sleep(0.0002)
        with self._lock:
            if self._items:
                item = self._items.popleft()
            else:
                self.violations.append("get() called on an empty buffer")
                item = EMPTY
        self._end()
        return item

    def assert_no_violations(self):
        assert not self.violations, (
            f"{len(self.violations)} buffer violation(s); first: "
            f"{self.violations[0]}"
        )


def make_pc():
    buf = GuardedBuffer()
    return ProducerConsumer(buf), buf


def run_session(n_producers=3, n_consumers=3, items_each=40, max_jitter_ms=0.5):
    """n_producers×items_each produces vs n_consumers×items_each consumes.

    Producer counts equal consumer counts, so a correct solution finishes
    every consume. Returns (buffer, produced item list, consumed item list).
    """
    pc, buf = make_pc()
    consumed = []
    consumed_lock = threading.Lock()
    runner = ThreadRunner()

    def producer(pid):
        for i in range(items_each):
            jitter(max_jitter_ms)
            pc.produce(pid * 1000 + i)

    def consumer():
        for _ in range(items_each):
            jitter(max_jitter_ms)
            item = pc.consume()
            with consumed_lock:
                consumed.append(item)

    for pid in range(n_producers):
        runner.spawn(producer, pid, name=f"producer-{pid}")
    for c in range(n_consumers):
        runner.spawn(consumer, name=f"consumer-{c}")
    runner.join_all(timeout=20)
    produced = [p * 1000 + i for p in range(n_producers) for i in range(items_each)]
    return buf, produced, consumed


def test_produce_never_waits_for_a_consumer():
    pc, buf = make_pc()
    for i in range(5):
        assert_completes(
            lambda i=i: pc.produce(i),
            timeout=2,
            msg="produce() must not block — the buffer is unbounded and "
            "no consumer is required",
        )
    buf.assert_no_violations()


def test_consume_blocks_on_empty_then_gets_the_item():
    pc, buf = make_pc()
    result = {}
    probe = assert_blocks(
        lambda: result.setdefault("item", pc.consume()),
        msg="consume() must block while the buffer is empty",
    )
    pc.produce(42)
    assert probe.wait(5), "consume() should return once an item is produced"
    assert result.get("item") == 42, f"consumed {result.get('item')!r}, not 42"
    buf.assert_no_violations()


def test_no_lost_or_duplicated_items():
    # Deliberately NOT checked: FIFO order. The contract makes the buffer
    # a bag — any item satisfies a consume. Only the multiset must match.
    buf, produced, consumed = run_session()
    assert sorted(consumed) == sorted(produced), (
        "consumed items != produced items (lost, duplicated, or "
        f"empty-buffer sentinel): {sorted(consumed)[:8]}…"
    )


def test_buffer_access_is_exclusive():
    buf, produced, consumed = run_session()
    buf.assert_no_violations()
    assert sorted(consumed) == sorted(produced)


def test_stress_with_jitter_on_both_sides():
    for _ in range(3):
        buf, produced, consumed = run_session(
            n_producers=4, n_consumers=4, items_each=25, max_jitter_ms=1.0
        )
        buf.assert_no_violations()
        assert sorted(consumed) == sorted(produced)
