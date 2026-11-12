#include "doctest/doctest.h"
#include "tta/RunRecorder.h"

#include <algorithm>
#include <stdexcept>

using namespace tta;

TEST_CASE("run clock yields one frame per 50 ms, starting at t = 0") {
    RunRecorder rec({0, 1}, 10.0);
    CHECK(rec.advance(0.0) == 1);
    CHECK(rec.advance(0.016) == 0);
    CHECK(rec.advance(0.034) == 1); // 50 ms
    CHECK(rec.advance(0.120) == 2); // 170 ms -> frames at 100, 150
    CHECK(rec.elapsedMs() == 170);
    CHECK_THROWS_AS(rec.advance(-0.01), std::invalid_argument);
}

TEST_CASE("hits carry the trap's id and owner at the current time") {
    RunRecorder rec({0, 1}, 10.0);
    rec.advance(1.25);
    Placement trap;
    trap.id = 7;
    trap.owner = 1;
    rec.recordHit(trap, 0);
    REQUIRE(rec.replay().events().size() == 1);
    const ReplayEvent& e = rec.replay().events()[0];
    CHECK(e.type == ReplayEventType::Hit);
    CHECK(e.timeMs == 1250);
    CHECK(e.victim == 0);
    CHECK(e.trapId == 7);
    CHECK(e.trapOwner == 1);

    Placement cam;
    cam.isCamera = true;
    CHECK_THROWS_AS(rec.recordHit(cam, 0), std::invalid_argument);
}

TEST_CASE("timeout smites unfinished runners exactly once") {
    RunRecorder rec({0, 1, 2}, 2.0);
    const std::vector<int> unfinished{0, 2};
    rec.advance(1.9);
    CHECK_FALSE(rec.timedOut());
    CHECK(rec.takeSmites(unfinished).empty());
    rec.advance(0.2);
    CHECK(rec.timedOut());
    CHECK(rec.takeSmites(unfinished) == unfinished);
    CHECK(rec.takeSmites(unfinished).empty());
    const auto& ev = rec.replay().events();
    REQUIRE(ev.size() == 2);
    CHECK(ev[0].type == ReplayEventType::Smite);
    CHECK(ev[0].timeMs == 2000);
    CHECK(ev[1].victim == 2);
    CHECK(ev[1].trapId == kNoTrap);
}

TEST_CASE("bad run length is rejected") {
    CHECK_THROWS_AS(RunRecorder({0}, 0.0), std::invalid_argument);
}

TEST_CASE("roundInput turns hits into wipeouts and scores with the Core rules") {
    RunRecorder rec({0, 1, 2}, 2.0);
    rec.advance(0.5);
    Placement banana;
    banana.id = 3;
    banana.owner = 2;
    banana.trap = TrapType::Banana;
    rec.recordHit(banana, 0);
    rec.recordHit(banana, 1);
    rec.advance(2.0);
    rec.takeSmites({0});

    const RoundInput round = rec.roundInput({1});
    CHECK(round.players == std::set<int>{0, 1, 2});
    CHECK(round.finishers == std::set<int>{1});
    REQUIRE(round.wipeouts.size() == 2); // the smite is not a wipeout
    CHECK(round.wipeouts[0].victim == 0);
    CHECK(round.wipeouts[0].trapOwner == 2);
    CHECK(round.wipeouts[0].trap == TrapType::Banana);

    const auto pts = scoreRound(round);
    CHECK(pts.at(1) == kGatePoints);
    CHECK(pts.at(2) == 2 * std::max(kMinTrapPoints, double(trapSpec(TrapType::Banana).cost)));
    CHECK(pts.at(0) == 0.0);
}

TEST_CASE("roundInput rejects a finisher who didn't run") {
    RunRecorder rec({0, 1}, 1.0);
    CHECK_THROWS_AS(scoreRound(rec.roundInput({5})), std::invalid_argument);
}
