#include "koan_test.hpp"
#include "river.hpp"

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace koans;

namespace {

struct Bench {
    std::shared_ptr<EventLog> log = std::make_shared<EventLog>();
    std::shared_ptr<Boat> boat;

    Bench() {
        auto l = log;
        boat = std::make_shared<Boat>(BoatHooks{
            [l](const std::string& kind) {
                jitter(2);
                l->record("board:" + kind);
            },
            [l](const std::string&) { l->record("row"); }});
    }

    std::vector<std::string> boards() const {
        std::vector<std::string> out;
        for (const auto& e : log->events())
            if (e.rfind("board:", 0) == 0) out.push_back(e.substr(6));
        return out;
    }

    std::size_t rows() const { return log->count("row"); }
};

void assert_boatloads_legal(const std::vector<std::string>& boards) {
    KOAN_ASSERT_MSG(boards.size() % 4 == 0,
                    "board count " + std::to_string(boards.size()) +
                        " is not a multiple of 4");
    for (std::size_t i = 0; i < boards.size(); i += 4) {
        int h = 0, s = 0;
        for (std::size_t j = i; j < i + 4; ++j) {
            if (boards[j] == "hacker") ++h;
            if (boards[j] == "serf") ++s;
        }
        bool legal = (h == 4 && s == 0) || (h == 0 && s == 4) || (h == 2 && s == 2);
        KOAN_ASSERT_MSG(legal, "boatload #" + std::to_string(i / 4) +
                                   " is illegal: " + std::to_string(h) +
                                   " hackers + " + std::to_string(s) + " serfs");
    }
}

// Each boatload: exactly 4 boards, then exactly one row, no overlap.
void assert_full_pattern(const std::vector<std::string>& events) {
    int pending = 0;
    for (const auto& e : events) {
        if (e.rfind("board:", 0) == 0) {
            ++pending;
            KOAN_ASSERT_MSG(pending <= 4,
                            "a fifth passenger boarded before the boat sailed");
        } else if (e == "row") {
            KOAN_ASSERT_MSG(pending == 4,
                            "row_boat fired before all four passengers boarded");
            pending = 0;
        }
    }
    KOAN_ASSERT_MSG(pending == 0, "a boatload boarded but never sailed");
}

Bench run_mix(int n_hackers, int n_serfs,
              std::chrono::milliseconds join_timeout = 15000ms) {
    Bench bench;
    ThreadRunner runner;
    std::vector<char> kinds(static_cast<std::size_t>(n_hackers), 'h');
    kinds.insert(kinds.end(), static_cast<std::size_t>(n_serfs), 's');
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(kinds.begin(), kinds.end(), rng);
    auto boat = bench.boat;
    for (char kind : kinds) {
        runner.spawn([boat, kind] {
            jitter();
            if (kind == 'h')
                boat->hacker_arrives();
            else
                boat->serf_arrives();
        });
    }
    runner.join_all(join_timeout);
    return bench;
}

}  // namespace

KOAN_TEST(three_and_one_never_sails) {
    Bench bench;
    auto boat = bench.boat;
    auto log = bench.log;
    auto done = std::make_shared<EventLog>();

    auto spawn_rider = [boat, done](const std::string& kind) {
        // Detached: the lone serf stays parked past the end of the test,
        // so everything it touches is shared_ptr-owned.
        std::thread([boat, done, kind] {
            try {
                if (kind == "hacker")
                    boat->hacker_arrives();
                else
                    boat->serf_arrives();
                done->record(kind);
            } catch (const std::exception& e) {
                done->record(std::string("error: ") + e.what());
            }
        }).detach();
    };

    for (int i = 0; i < 3; ++i) spawn_rider("hacker");
    spawn_rider("serf");
    std::this_thread::sleep_for(400ms);  // give an illegal crew every chance
    KOAN_ASSERT_MSG(log->events().empty(),
                    "3 hackers + 1 serf must not board anything, saw " +
                        log->joined());
    KOAN_ASSERT_MSG(done->events().empty(),
                    "nobody may cross in an illegal combination: " +
                        done->joined());
    // A fourth hacker makes an all-hacker crew possible; the serf stays.
    spawn_rider("hacker");
    eventually([done] { return done->count("hacker") == 4; }, 5000ms,
               "four hackers should sail together once the fourth arrives");
    auto boards = bench.boards();
    KOAN_ASSERT_MSG(boards == std::vector<std::string>(4, "hacker"),
                    "expected exactly the 4 hackers to board");
    KOAN_ASSERT_EQ(bench.rows(), std::size_t{1});
    std::this_thread::sleep_for(300ms);
    KOAN_ASSERT_MSG(done->count("serf") == 0,
                    "the lone serf must keep waiting ashore");
}

KOAN_TEST(pairs_combination_sails) {
    auto bench = run_mix(2, 2, 5000ms);
    auto boards = bench.boards();
    std::sort(boards.begin(), boards.end());
    KOAN_ASSERT(boards ==
                (std::vector<std::string>{"hacker", "hacker", "serf", "serf"}));
    KOAN_ASSERT_EQ(bench.rows(), std::size_t{1});
}

KOAN_TEST(four_of_a_kind_sails) {
    auto bench = run_mix(0, 4, 5000ms);
    KOAN_ASSERT(bench.boards() == std::vector<std::string>(4, "serf"));
    KOAN_ASSERT_EQ(bench.rows(), std::size_t{1});
}

KOAN_TEST(rowing_after_all_board) {
    struct Mix { int h, s; };
    for (Mix mix : {Mix{2, 2}, Mix{4, 0}, Mix{0, 4}}) {
        auto bench = run_mix(mix.h, mix.s, 5000ms);
        auto events = bench.log->events();
        KOAN_ASSERT_MSG(bench.rows() == 1, "exactly one rower per boatload");
        long row_index = -1;
        std::vector<long> board_indexes;
        for (std::size_t i = 0; i < events.size(); ++i) {
            if (events[i] == "row") row_index = static_cast<long>(i);
            if (events[i].rfind("board:", 0) == 0)
                board_indexes.push_back(static_cast<long>(i));
        }
        KOAN_ASSERT_EQ(board_indexes.size(), std::size_t{4});
        for (long b : board_indexes)
            KOAN_ASSERT_MSG(b < row_index,
                            "row_boat must come after all four boards: " +
                                bench.log->joined());
    }
}

KOAN_TEST(boatloads_do_not_interleave) {
    auto bench = run_mix(8, 8);
    assert_boatloads_legal(bench.boards());
    KOAN_ASSERT_EQ(bench.rows(), std::size_t{4});
    assert_full_pattern(bench.log->events());
}

KOAN_TEST(stress) {
    auto bench = run_mix(16, 16, 20000ms);
    KOAN_ASSERT_EQ(bench.boards().size(), std::size_t{32});
    assert_boatloads_legal(bench.boards());
    KOAN_ASSERT_EQ(bench.rows(), std::size_t{8});
    assert_full_pattern(bench.log->events());
}
