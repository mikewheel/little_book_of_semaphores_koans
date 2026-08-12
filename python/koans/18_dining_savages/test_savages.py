import time

from koan_utils import ThreadRunner, assert_completes, jitter

from savages import Village


class InstrumentedPot:
    """A pot that notices misuse.

    Deliberately NOT thread-safe: serializing access to the pot is the
    solution's job, so a race here surfaces as a violation or a wrong count
    rather than being papered over by a lock.
    """

    def __init__(self, m):
        self.m = m
        self.servings = 0
        self.refill_count = 0
        self.served_count = 0
        self.violations = []

    def put_servings(self, m):
        if self.servings != 0:
            self.violations.append(
                f"cook refilled a pot that still held {self.servings} serving(s)"
            )
        if m != self.m:
            self.violations.append(f"cook refilled with {m}, expected {self.m}")
        time.sleep(0.0002)  # widen the race window on purpose
        self.servings = m
        self.refill_count += 1

    def get_serving(self):
        s = self.servings
        if s <= 0:
            self.violations.append("a diner took a serving from an empty pot")
        time.sleep(0.0002)  # widen the race window on purpose
        self.servings = s - 1
        self.served_count += 1

    def assert_no_violations(self):
        assert not self.violations, (
            f"{len(self.violations)} pot violation(s); first: {self.violations[0]}"
        )


def run_village(m, n_savages, meals_each, max_jitter_ms=0.5):
    pot = InstrumentedPot(m)
    village = Village(m, pot)
    village.start_cook()
    runner = ThreadRunner()

    def savage():
        for _ in range(meals_each):
            jitter(max_jitter_ms)
            village.dine()

    for _ in range(n_savages):
        runner.spawn(savage, name="savage")
    runner.join_all(timeout=20)
    return pot


def test_pot_never_misused():
    pot = run_village(m=4, n_savages=6, meals_each=10)
    pot.assert_no_violations()


def test_all_meals_served():
    pot = run_village(m=4, n_savages=6, meals_each=10)
    assert pot.served_count == 60, (
        f"expected 60 servings taken, saw {pot.served_count}"
    )


def test_cook_called_right_number_of_times():
    # 60 one-serving meals from a pot of 4 → exactly ceil(60/4) == 15
    # refills: one each time a diner finds the pot empty, never on spec.
    pot = run_village(m=4, n_savages=6, meals_each=10)
    assert pot.refill_count == 15, (
        f"expected exactly 15 refills for 60 meals (m=4), saw "
        f"{pot.refill_count} — is the cook refilling only on demand?"
    )


def test_cook_sleeps_until_needed():
    pot = InstrumentedPot(4)
    village = Village(4, pot)
    village.start_cook()
    time.sleep(0.3)  # nobody is hungry yet
    assert pot.refill_count == 0, (
        "the cook refilled the pot before any diner asked — he should sleep"
    )
    # The first diner finds the pot empty and must wake the cook.
    assert_completes(village.dine, timeout=5, msg="the first dine() never returned")
    assert pot.refill_count == 1
    assert pot.served_count == 1


def test_stress_with_jitter():
    # 40 meals from a pot of 3 → exactly ceil(40/3) == 14 refills.
    pot = run_village(m=3, n_savages=8, meals_each=5, max_jitter_ms=2.0)
    pot.assert_no_violations()
    assert pot.served_count == 40
    assert pot.refill_count == 14, (
        f"expected exactly 14 refills for 40 meals (m=3), saw {pot.refill_count}"
    )
