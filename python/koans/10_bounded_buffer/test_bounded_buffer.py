import threading
import time
from collections import deque

from koan_utils import ThreadRunner, assert_blocks, assert_completes, jitter

from bounded_buffer import BoundedBuffer

# Sentinel returned by GuardedBuffer.get() on an empty buffer, instead of
# crashing the calling thread. An int so result multisets stay sortable.
EMPTY = -999_999


class GuardedBuffer:
    """An instrumented buffer that DETECTS concurrent access and overfill.

    Same detector as koan 09 (an "operation in flight" count plus a dwell
    to widen race windows), with one addition: it tracks its size and the
    largest size it ever reached, so tests can prove the capacity bound
    held. get() on an empty buffer records a violation and returns EMPTY.
    """

    def __init__(self):
        self._items = deque()
        self._lock = threading.Lock()
        self._in_flight = 0
        self.max_size = 0
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
            self.max_size = max(self.max_size, len(self._items))
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


def make_bb(capacity):
    buf = GuardedBuffer()
    return BoundedBuffer(buf, capacity), buf


def run_session(capacity, n_producers=3, n_consumers=3, items_each=40,
                producer_jitter_ms=0.2, consumer_jitter_ms=0.5):
    """Equal produce/consume counts; consumers slightly slower by default so
    the buffer runs near-full and the capacity bound actually gets tested.
    Returns (buffer, produced items, consumed items)."""
    bb, buf = make_bb(capacity)
    consumed = []
    consumed_lock = threading.Lock()
    runner = ThreadRunner()

    def producer(pid):
        for i in range(items_each):
            jitter(producer_jitter_ms)
            bb.produce(pid * 1000 + i)

    def consumer():
        for _ in range(items_each):
            jitter(consumer_jitter_ms)
            item = bb.consume()
            with consumed_lock:
                consumed.append(item)

    for pid in range(n_producers):
        runner.spawn(producer, pid, name=f"producer-{pid}")
    for c in range(n_consumers):
        runner.spawn(consumer, name=f"consumer-{c}")
    runner.join_all(timeout=25)
    produced = [p * 1000 + i for p in range(n_producers) for i in range(items_each)]
    return buf, produced, consumed


def test_produce_completes_while_there_is_space():
    bb, buf = make_bb(8)
    for i in range(8):
        assert_completes(
            lambda i=i: bb.produce(i),
            timeout=2,
            msg="produce() must not block while the buffer has space",
        )
    buf.assert_no_violations()


def test_consume_blocks_on_empty_then_gets_the_item():
    bb, buf = make_bb(4)
    result = {}
    probe = assert_blocks(
        lambda: result.setdefault("item", bb.consume()),
        msg="consume() must block while the buffer is empty",
    )
    bb.produce(42)
    assert probe.wait(5), "consume() should return once an item is produced"
    assert result.get("item") == 42, f"consumed {result.get('item')!r}, not 42"
    buf.assert_no_violations()


def test_producer_blocks_when_full():
    bb, buf = make_bb(3)
    for i in range(3):
        assert_completes(
            lambda i=i: bb.produce(i),
            timeout=2,
            msg="produce() must not block while the buffer has space",
        )
    probe = assert_blocks(
        lambda: bb.produce(99),
        msg="produce() must block once the buffer holds `capacity` items",
    )
    # A consumer must be able to get in even while a producer is waiting —
    # if this times out, the producer is probably sleeping INSIDE the mutex.
    first = assert_completes(
        bb.consume,
        timeout=5,
        msg="consume() should not be locked out by a blocked producer",
    )
    assert probe.wait(5), "the waiting producer should take the freed slot"
    rest = [
        assert_completes(bb.consume, timeout=5, msg="draining the buffer")
        for _ in range(3)
    ]
    assert sorted([first] + rest) == [0, 1, 2, 99]
    assert buf.max_size <= 3, f"buffer grew to {buf.max_size} despite capacity 3"
    buf.assert_no_violations()


def test_no_lost_or_duplicated_items():
    # FIFO order is deliberately not required — only the multiset matters.
    buf, produced, consumed = run_session(capacity=7)
    assert sorted(consumed) == sorted(produced), (
        "consumed items != produced items (lost, duplicated, or "
        "empty-buffer sentinel)"
    )


def test_buffer_access_is_exclusive():
    buf, produced, consumed = run_session(capacity=7)
    buf.assert_no_violations()
    assert sorted(consumed) == sorted(produced)


def test_never_exceeds_capacity():
    buf, produced, consumed = run_session(
        capacity=5, n_producers=4, n_consumers=4, items_each=50,
        producer_jitter_ms=0.1, consumer_jitter_ms=1.0,
    )
    assert buf.max_size <= 5, (
        f"buffer reached {buf.max_size} items — capacity 5 was not enforced"
    )
    buf.assert_no_violations()
    assert sorted(consumed) == sorted(produced)


def test_stress_with_jitter_on_both_sides():
    for capacity in (2, 6):
        buf, produced, consumed = run_session(
            capacity=capacity, n_producers=4, n_consumers=4, items_each=20,
            producer_jitter_ms=1.0, consumer_jitter_ms=1.0,
        )
        assert buf.max_size <= capacity
        buf.assert_no_violations()
        assert sorted(consumed) == sorted(produced)
