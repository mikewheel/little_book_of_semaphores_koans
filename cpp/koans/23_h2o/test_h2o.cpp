#include "koan_test.hpp"
#include "h2o.hpp"

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

// A barrier wired to a shared bond log; bond() dawdles to widen windows.
std::shared_ptr<H2OBarrier> make_barrier(std::shared_ptr<EventLog> bonds) {
    return std::make_shared<H2OBarrier>(H2OHooks{
        [bonds](const std::string& kind) {
            jitter(2);  // a slow chemistry set exposes molecules that smear
            bonds->record(kind);
        }});
}

void assert_molecules_well_formed(const std::vector<std::string>& bonds) {
    KOAN_ASSERT_MSG(bonds.size() % 3 == 0,
                    "bond count " + std::to_string(bonds.size()) +
                        " is not a multiple of 3");
    for (std::size_t i = 0; i < bonds.size(); i += 3) {
        int h = 0, o = 0;
        for (std::size_t j = i; j < i + 3; ++j) {
            if (bonds[j] == "H") ++h;
            if (bonds[j] == "O") ++o;
        }
        KOAN_ASSERT_MSG(h == 2 && o == 1,
                        "molecule #" + std::to_string(i / 3) +
                            " is malformed: " + std::to_string(h) + " H + " +
                            std::to_string(o) + " O");
    }
}

std::vector<std::string> run_batch(int n_h, int n_o,
                                   std::chrono::milliseconds join_timeout) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    ThreadRunner runner;
    std::vector<char> kinds(static_cast<std::size_t>(n_h), 'H');
    kinds.insert(kinds.end(), static_cast<std::size_t>(n_o), 'O');
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(kinds.begin(), kinds.end(), rng);
    for (char kind : kinds) {
        runner.spawn([barrier, kind] {
            jitter();
            if (kind == 'H')
                barrier->hydrogen();
            else
                barrier->oxygen();
        });
    }
    runner.join_all(join_timeout);
    return bonds->events();
}

}  // namespace

KOAN_TEST(lone_hydrogen_blocks) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    assert_blocks([barrier] { barrier->hydrogen(); }, 300ms,
                  "a lone hydrogen (no full molecule available)");
}

KOAN_TEST(two_hydrogens_block) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    assert_blocks([barrier] { barrier->hydrogen(); }, 300ms,
                  "hydrogen #1 (no oxygen yet)");
    assert_blocks([barrier] { barrier->hydrogen(); }, 300ms,
                  "hydrogen #2 (still no oxygen)");
    KOAN_ASSERT_MSG(bonds->events().empty(),
                    "nothing may bond before a molecule is complete");
}

KOAN_TEST(lone_oxygen_blocks) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    assert_blocks([barrier] { barrier->oxygen(); }, 300ms,
                  "a lone oxygen (needs two hydrogens)");
}

KOAN_TEST(h_h_o_completes) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    auto p1 = assert_blocks([barrier] { barrier->hydrogen(); }, 300ms,
                            "hydrogen #1");
    auto p2 = assert_blocks([barrier] { barrier->hydrogen(); }, 300ms,
                            "hydrogen #2");
    assert_completes([barrier] { barrier->oxygen(); }, 5000ms,
                     "the completing oxygen");
    p1.assert_completed(5000ms, "hydrogen #1 once the molecule is complete");
    p2.assert_completed(5000ms, "hydrogen #2 once the molecule is complete");
    KOAN_ASSERT_EQ(bonds->count("H"), std::size_t{2});
    KOAN_ASSERT_EQ(bonds->count("O"), std::size_t{1});
}

KOAN_TEST(molecules_are_well_formed) {
    auto bonds = run_batch(20, 10, 15000ms);
    KOAN_ASSERT_EQ(bonds.size(), std::size_t{30});
    assert_molecules_well_formed(bonds);
}

KOAN_TEST(no_partial_molecule_left) {
    auto bonds = std::make_shared<EventLog>();
    auto barrier = make_barrier(bonds);
    auto done = std::make_shared<EventLog>();

    auto spawn_atom = [barrier, done](char kind) {
        // Detached: one hydrogen stays parked past the end of the test, so
        // everything it touches is owned by shared_ptr captured by value.
        std::thread([barrier, done, kind] {
            try {
                if (kind == 'H')
                    barrier->hydrogen();
                else
                    barrier->oxygen();
                done->record(std::string(1, kind));
            } catch (const std::exception& e) {
                done->record(std::string("error: ") + e.what());
            }
        }).detach();
    };

    for (int i = 0; i < 5; ++i) spawn_atom('H');
    for (int i = 0; i < 2; ++i) spawn_atom('O');

    eventually([done] { return done->events().size() >= 6; }, 5000ms,
               "5 H + 2 O should yield two complete molecules");
    std::this_thread::sleep_for(300ms);  // let a leftover atom try to leak
    KOAN_ASSERT_MSG(bonds->events().size() == 6,
                    "exactly 6 bonds expected (2 molecules), saw " +
                        bonds->joined());
    KOAN_ASSERT_MSG(done->count("H") == 4 && done->count("O") == 2,
                    "exactly 4 H and 2 O should have returned, saw " +
                        done->joined());
    // One more H and one more O rescue the stranded hydrogen.
    spawn_atom('H');
    spawn_atom('O');
    eventually([done] { return done->events().size() == 9; }, 5000ms,
               "the stranded hydrogen never got its molecule");
    assert_molecules_well_formed(bonds->events());
}

KOAN_TEST(stress) {
    for (int round = 0; round < 3; ++round) {
        auto bonds = run_batch(40, 20, 20000ms);
        KOAN_ASSERT_EQ(bonds.size(), std::size_t{60});
        assert_molecules_well_formed(bonds);
    }
}
