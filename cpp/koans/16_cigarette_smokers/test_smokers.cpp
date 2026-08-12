#include "koan_test.hpp"
#include "smokers.hpp"

#include <array>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>

using namespace koans;

namespace {

// Each pair of ingredient indices the agent can put out, and who must
// smoke (the complement's owner). 0=tobacco, 1=paper, 2=match.
struct Round {
    int first, second, complement;
};
constexpr std::array<Round, 3> kRounds{{
    {0, 1, 2},  // tobacco+paper  -> match owner
    {1, 2, 0},  // paper+match    -> tobacco owner
    {0, 2, 1},  // tobacco+match  -> paper owner
}};

struct Rig {
    std::shared_ptr<AgentTable> table = std::make_shared<AgentTable>();
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();

    void start() {
        auto log_ = log;
        Smokers smokers(table, [log_](const std::string& kind) {
            log_->record(kind);
        });
        smokers.start();
    }

    // One agent turn: wait to be signaled, put out two ingredients, then
    // wait (bounded!) for exactly one smoke of the right kind.
    void play_round(const Round& r, std::size_t served_before) {
        KOAN_ASSERT_MSG(table->agent_sem.try_acquire_for(5s),
                        "the agent never got the go-ahead — is agent_sem "
                        "released after each smoke?");
        table->ingredient_sem(r.first).release();
        table->ingredient_sem(r.second).release();
        eventually(
            [&] { return log->events().size() >= served_before + 1; }, 5000ms,
            std::string("nobody smoked after the agent put out ") +
                kIngredients[r.first] + "+" + kIngredients[r.second] +
                " — deadlock? (smokers must not grab ingredients they "
                "cannot use)");
        auto events = log->events();
        KOAN_ASSERT_MSG(events.size() == served_before + 1,
                        "expected exactly one smoke per round, saw " +
                            log->joined());
        KOAN_ASSERT_MSG(events.back() == kIngredients[r.complement],
                        std::string("the agent put out ") +
                            kIngredients[r.first] + "+" +
                            kIngredients[r.second] + "; the smoker owning " +
                            kIngredients[r.complement] +
                            " had to smoke, but " + events.back() + " did");
    }
};

}  // namespace

KOAN_TEST(no_smoke_before_the_agent_acts) {
    Rig rig;
    rig.start();
    std::this_thread::sleep_for(300ms);  // every chance to misbehave
    KOAN_ASSERT_MSG(rig.log->events().empty(),
                    "smokers smoked " + rig.log->joined() +
                        " before any ingredients existed");
}

// Cycle deterministically through all three pairs, 30 rounds.
KOAN_TEST(only_matching_smoker_smokes) {
    Rig rig;
    rig.start();
    for (std::size_t r = 0; r < 30; ++r)
        rig.play_round(kRounds[r % 3], r);
}

KOAN_TEST(no_spurious_smokes) {
    Rig rig;
    rig.start();
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick(0, 2);
    constexpr std::size_t rounds = 15;
    for (std::size_t r = 0; r < rounds; ++r)
        rig.play_round(kRounds[pick(rng)], r);
    // The final agent_sem signal must be there, and then: silence.
    KOAN_ASSERT(rig.table->agent_sem.try_acquire_for(5s));
    std::this_thread::sleep_for(250ms);
    KOAN_ASSERT_MSG(rig.log->events().size() == rounds,
                    "smoke count changed after the agent stopped: " +
                        rig.log->joined());
}

KOAN_TEST(stress_random_pairs) {
    Rig rig;
    rig.start();
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> pick(0, 2);
    std::array<std::size_t, 3> tally{};
    for (std::size_t r = 0; r < 60; ++r) {
        const Round& round = kRounds[pick(rng)];
        ++tally[static_cast<std::size_t>(round.complement)];
        rig.play_round(round, r);
    }
    for (int k = 0; k < 3; ++k) {
        KOAN_ASSERT_MSG(
            rig.log->count(kIngredients[k]) == tally[static_cast<std::size_t>(k)],
            std::string("the ") + kIngredients[k] + "-owning smoker smoked " +
                std::to_string(rig.log->count(kIngredients[k])) +
                " times; the agent's pairs entitled it to " +
                std::to_string(tally[static_cast<std::size_t>(k)]));
    }
}
