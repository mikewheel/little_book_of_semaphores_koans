#include "koan_test.hpp"
#include "generalized_smokers.hpp"

#include <array>
#include <memory>
#include <random>
#include <string>
#include <thread>

using namespace koans;

namespace {

// Each pair of ingredient indices the agent can put out, and who must
// smoke it (the complement's owner). 0=tobacco, 1=paper, 2=match.
struct Pair {
    int first, second, complement;
};
constexpr std::array<Pair, 3> kPairs{{
    {0, 1, 2},  // tobacco+paper  -> match owner
    {1, 2, 0},  // paper+match    -> tobacco owner
    {0, 2, 1},  // tobacco+match  -> paper owner
}};

struct Rig {
    std::shared_ptr<AgentTable> table = std::make_shared<AgentTable>();
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();

    void start() {
        auto log_ = log;
        GeneralizedSmokers smokers(
            table, [log_](const std::string& kind) { log_->record(kind); });
        smokers.start();
    }

    std::size_t total() const { return log->events().size(); }
};

// Fire `rounds` random pairs; returns how often each owner must smoke.
std::array<std::size_t, 3> blast_pairs(Rig& rig, std::size_t rounds,
                                       int pause_ms) {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick(0, 2);
    std::array<std::size_t, 3> owed{};
    for (std::size_t r = 0; r < rounds; ++r) {
        const Pair& pair = kPairs[pick(rng)];
        ++owed[static_cast<std::size_t>(pair.complement)];
        rig.table->ingredient_sem(pair.first).release();
        rig.table->ingredient_sem(pair.second).release();
        if (pause_ms) jitter(pause_ms);
    }
    return owed;
}

void assert_all_smoked(const Rig& rig, const std::array<std::size_t, 3>& owed,
                       std::size_t rounds) {
    eventually([&] { return rig.total() >= rounds; }, 10000ms,
               "not every cigarette was smoked — ingredients got lost "
               "(noted down, or overwritten?)");
    std::this_thread::sleep_for(250ms);  // any over-smoking shows up now
    KOAN_ASSERT_MSG(rig.total() == rounds,
                    "expected exactly " + std::to_string(rounds) +
                        " cigarettes, saw " + std::to_string(rig.total()) +
                        ": " + rig.log->joined());
    for (int k = 0; k < 3; ++k) {
        auto got = rig.log->count(kIngredients[k]);
        KOAN_ASSERT_MSG(
            got == owed[static_cast<std::size_t>(k)],
            std::string("conservation violated for ") + kIngredients[k] +
                ": its owner smoked " + std::to_string(got) +
                " times but the released pairs entitled it to " +
                std::to_string(owed[static_cast<std::size_t>(k)]) +
                " — an ingredient was lost or double-counted");
    }
}

}  // namespace

KOAN_TEST(no_smoke_without_ingredients) {
    Rig rig;
    rig.start();
    std::this_thread::sleep_for(300ms);  // every chance to misbehave
    KOAN_ASSERT_MSG(rig.log->events().empty(),
                    "smokers smoked " + rig.log->joined() +
                        " before any ingredients existed");
}

KOAN_TEST(all_cigarettes_eventually_smoked) {
    Rig rig;
    rig.start();
    constexpr std::size_t rounds = 60;
    auto owed = blast_pairs(rig, rounds, 1);
    assert_all_smoked(rig, owed, rounds);
}

// Fixed diet: released ingredient counts pin down every smoke tally.
KOAN_TEST(per_smoker_counts_match_conservation) {
    Rig rig;
    rig.start();
    std::vector<Pair> schedule;
    for (int i = 0; i < 10; ++i) schedule.push_back(kPairs[0]);
    for (int i = 0; i < 8; ++i) schedule.push_back(kPairs[1]);
    for (int i = 0; i < 6; ++i) schedule.push_back(kPairs[2]);
    std::shuffle(schedule.begin(), schedule.end(),
                 std::mt19937{std::random_device{}()});
    for (const Pair& pair : schedule) {
        rig.table->ingredient_sem(pair.first).release();
        rig.table->ingredient_sem(pair.second).release();
        jitter(1);
    }
    eventually([&] { return rig.total() >= schedule.size(); }, 10000ms);
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_EQ(rig.log->count("match"), 10u);    // tobacco+paper rounds
    KOAN_ASSERT_EQ(rig.log->count("tobacco"), 8u);   // paper+match rounds
    KOAN_ASSERT_EQ(rig.log->count("paper"), 6u);     // tobacco+match rounds
}

// Twenty copies of one pair dumped at once (then the next pair type):
// many duplicate tokens are pending together, and every one must be
// remembered. (Bursts of a single pair type keep the bookkeeping
// schedule-independent; a boolean scoreboard still drowns.)
KOAN_TEST(burst_stress) {
    Rig rig;
    rig.start();
    constexpr std::size_t per_type = 20;
    std::size_t smoked = 0;
    for (const Pair& pair : kPairs) {
        for (std::size_t i = 0; i < per_type; ++i) {
            rig.table->ingredient_sem(pair.first).release();
            rig.table->ingredient_sem(pair.second).release();
        }
        smoked += per_type;
        eventually([&] { return rig.total() >= smoked; }, 10000ms,
                   std::string("a burst of ") + std::to_string(per_type) +
                       " x " + kIngredients[pair.first] + "+" +
                       kIngredients[pair.second] +
                       " was not fully smoked — duplicate ingredients were "
                       "forgotten");
    }
    std::this_thread::sleep_for(250ms);  // any over-smoking shows up now
    KOAN_ASSERT_EQ(rig.total(), smoked);
    for (const Pair& pair : kPairs) {
        auto got = rig.log->count(kIngredients[pair.complement]);
        KOAN_ASSERT_MSG(got == per_type,
                        std::string(kIngredients[pair.complement]) +
                            "'s owner should have smoked " +
                            std::to_string(per_type) + ", got " +
                            std::to_string(got));
    }
}
