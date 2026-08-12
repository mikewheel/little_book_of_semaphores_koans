"""Shared helpers for the semaphore koan test suites.

These utilities give the tests a vocabulary for talking about concurrent
events: what happened, in what order, and how many things were happening at
once. They also provide guard rails so a deadlocked (or unimplemented)
solution fails a test quickly instead of hanging the suite.

Conventions used throughout the koans:

- Worker threads are always daemons, so a thread that is stuck waiting on a
  semaphore can never keep the process alive.
- Every join/wait has a timeout. A timeout usually means your solution
  deadlocked or forgot a signal.
- Tests inject random tiny sleeps (``jitter``) to shake out orderings that
  only appear under unlucky scheduling. A passing run is strong evidence,
  not proof — that's the nature of concurrency testing.
"""

import random
import threading
import time

DEFAULT_TIMEOUT = 10.0


class WorkerError(AssertionError):
    """An exception raised inside a worker thread, re-raised in the test."""


class ThreadRunner:
    """Spawns worker threads, collects their exceptions, joins with timeouts."""

    def __init__(self):
        self._threads = []
        self._errors = []
        self._lock = threading.Lock()

    def spawn(self, fn, *args, name=None):
        def wrapper():
            try:
                fn(*args)
            except BaseException as exc:  # reported to the test thread
                with self._lock:
                    self._errors.append((threading.current_thread().name, exc))

        t = threading.Thread(target=wrapper, name=name or fn.__name__, daemon=True)
        self._threads.append(t)
        t.start()
        return t

    def join_all(self, timeout=DEFAULT_TIMEOUT):
        """Join every spawned thread.

        Fails fast if any worker raised, and fails with a thread listing if
        some workers are still stuck when the timeout expires.
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self.raise_worker_errors()
            if all(not t.is_alive() for t in self._threads):
                self.raise_worker_errors()
                return
            time.sleep(0.01)
        self.raise_worker_errors()
        stuck = [t.name for t in self._threads if t.is_alive()]
        if stuck:
            raise AssertionError(
                "these threads never finished (deadlock, or a missing signal?): "
                f"{stuck}"
            )

    def raise_worker_errors(self):
        with self._lock:
            errors = list(self._errors)
        if errors:
            name, exc = errors[0]
            raise WorkerError(f"worker thread {name!r} raised: {exc!r}") from exc


class EventLog:
    """A thread-safe, ordered record of named events."""

    def __init__(self):
        self._events = []
        self._cond = threading.Condition()

    def record(self, label):
        with self._cond:
            self._events.append(label)
            self._cond.notify_all()

    def events(self):
        with self._cond:
            return list(self._events)

    def count(self, label):
        return self.events().count(label)

    def index(self, label, occurrence=0):
        """Index of the nth occurrence of ``label``; fails if absent."""
        seen = 0
        events = self.events()
        for i, e in enumerate(events):
            if e == label:
                if seen == occurrence:
                    return i
                seen += 1
        raise AssertionError(
            f"event {label!r} (occurrence {occurrence}) not found in {events}"
        )

    def assert_before(self, first, second):
        """Assert every occurrence of ``first`` precedes every ``second``."""
        events = self.events()
        last_first = max(i for i, e in enumerate(events) if e == first)
        first_second = min(i for i, e in enumerate(events) if e == second)
        if last_first > first_second:
            raise AssertionError(
                f"expected all {first!r} events before any {second!r} event; "
                f"log was {events}"
            )

    def wait_for_count(self, label, n, timeout=DEFAULT_TIMEOUT):
        """Block until ``label`` has been recorded ``n`` times."""
        deadline = time.monotonic() + timeout
        with self._cond:
            while self._events.count(label) < n:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise AssertionError(
                        f"timed out waiting for {n} x {label!r}; "
                        f"saw {self._events.count(label)} in {self._events}"
                    )
                self._cond.wait(remaining)


class OverlapTracker:
    """Tracks how many threads are inside named sections at the same time.

    Threads call ``enter(category)`` / ``exit(category)`` (or use
    ``section(category)`` as a context manager). ``enter`` returns a snapshot
    of the occupancy *including the caller*, so koan-specific invariants
    ("no writer while readers present") can be checked at the moment of entry.
    """

    def __init__(self):
        self._lock = threading.Lock()
        self._current = {}
        self._max = {}
        self._max_combined = 0
        self.violations = []

    def enter(self, category):
        with self._lock:
            self._current[category] = self._current.get(category, 0) + 1
            self._max[category] = max(
                self._max.get(category, 0), self._current[category]
            )
            self._max_combined = max(self._max_combined, sum(self._current.values()))
            return dict(self._current)

    def exit(self, category):
        with self._lock:
            self._current[category] -= 1

    def section(self, category, dwell=0.0):
        tracker = self

        class _Section:
            def __enter__(self):
                self.snapshot = tracker.enter(category)
                if dwell:
                    time.sleep(dwell)
                return self.snapshot

            def __exit__(self, *exc):
                tracker.exit(category)
                return False

        return _Section()

    def violate(self, message):
        with self._lock:
            self.violations.append(message)

    def assert_no_violations(self):
        with self._lock:
            if self.violations:
                raise AssertionError(
                    f"{len(self.violations)} invariant violation(s); first: "
                    f"{self.violations[0]}"
                )

    def max_concurrent(self, category):
        with self._lock:
            return self._max.get(category, 0)

    def max_combined(self):
        with self._lock:
            return self._max_combined

    def current(self, category):
        with self._lock:
            return self._current.get(category, 0)


def assert_blocks(fn, timeout=0.3, msg=None):
    """Assert that ``fn()`` does NOT return (or raise) within ``timeout``.

    Returns an Event that is set if/when ``fn`` eventually completes, so the
    test can later unblock it and assert it finished. The probe thread is a
    daemon: if the test ends while it is still blocked, it dies with the
    process. Make sure anything ``fn`` touches stays alive for the whole test.
    """
    done = threading.Event()
    outcome = {}

    def probe():
        try:
            outcome["result"] = fn()
        except BaseException as exc:
            outcome["error"] = exc
        done.set()

    threading.Thread(target=probe, daemon=True).start()
    if done.wait(timeout):
        if "error" in outcome:
            raise AssertionError(
                msg or f"expected the call to block, but it raised {outcome['error']!r}"
            ) from outcome.get("error")
        raise AssertionError(msg or "expected the call to block, but it returned")
    return done


def assert_completes(fn, timeout=DEFAULT_TIMEOUT, msg=None):
    """Assert that ``fn()`` returns within ``timeout``. Returns its result."""
    done = threading.Event()
    outcome = {}

    def probe():
        try:
            outcome["result"] = fn()
        except BaseException as exc:
            outcome["error"] = exc
        done.set()

    threading.Thread(target=probe, daemon=True).start()
    if not done.wait(timeout):
        raise AssertionError(
            msg or "the call never completed (deadlock, or a missing signal?)"
        )
    if "error" in outcome:
        raise WorkerError(f"call raised: {outcome['error']!r}") from outcome["error"]
    return outcome["result"]


def eventually(predicate, timeout=DEFAULT_TIMEOUT, interval=0.005, msg=None):
    """Poll until ``predicate()`` is true; fail if the timeout expires."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(interval)
    raise AssertionError(msg or "condition never became true")


def jitter(max_ms=2.0):
    """Sleep a random sliver of time to provoke unlucky schedules."""
    time.sleep(random.uniform(0, max_ms / 1000.0))
