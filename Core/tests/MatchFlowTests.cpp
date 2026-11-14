#include "doctest/doctest.h"
#include "tta/MatchFlow.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("players join only between rounds") {
    CHECK(canJoin(Phase::Lobby));
    CHECK(canJoin(Phase::Premiere));
    CHECK(canJoin(Phase::Podium));
    CHECK_FALSE(canJoin(Phase::Build));
    CHECK_FALSE(canJoin(Phase::Run));
    CHECK_FALSE(canJoin(Phase::Edit));
}

TEST_CASE("a leaver's traps keep a replay id after the runners") {
    Layout l;
    REQUIRE(l.placeTrap(7, TrapType::Hammer, 10, 0, 0) == PlaceResult::Ok);
    REQUIRE(l.placeTrap(9, TrapType::Banana, 20, 0, 0) == PlaceResult::Ok);
    REQUIRE(l.placeTrap(7, TrapType::Spinner, 30, 0, 0) == PlaceResult::Ok);
    REQUIRE(l.placeCamera(5, CameraType::Fixed, 5, 0, 0) == PlaceResult::Ok);
    CHECK(runPlayers({9, 3}, l) == std::vector<int>{9, 3, 7});
    CHECK(runPlayers({}, Layout{}).empty());
}

TEST_CASE("a leaver's trap still scores for them; voiding counts runners only") {
    const std::vector<int> ids{9, 3, 7}; // replay 0,1 ran; 2 left
    RoundInput run;
    run.players = {0, 1, 2};
    run.finishers = {0};
    run.wipeouts = {{1, 2, TrapType::Hammer}};
    const RoundInput round = toPlayerIds(run, ids, 2);
    CHECK(round.players == std::set<int>{9, 3});
    CHECK(round.absentOwners == std::set<int>{7});
    CHECK(round.finishers == std::set<int>{9});
    std::map<int, double> points = scoreRound(round);
    CHECK(points[9] == kGatePoints);
    CHECK(points[7] == trapSpec(TrapType::Hammer).cost);
    CHECK(points[3] == 0.0);

    run.finishers = {0, 1}; // every runner finished: void, even with a leaver
    CHECK(scoreRound(toPlayerIds(run, ids, 2))[7] == 0.0);
}

TEST_CASE("unknown replay ids are rejected") {
    RoundInput run;
    run.players = {0, 3};
    CHECK_THROWS_AS(toPlayerIds(run, {9, 3}, 2), std::invalid_argument);
}

TEST_CASE("an absent owner can't also be a runner") {
    RoundInput r;
    r.players = {1};
    r.absentOwners = {1};
    CHECK_THROWS_AS(scoreRound(r), std::invalid_argument);
}

TEST_CASE("three rounds run Build-Run-Edit-Premiere, then Podium; ratings add up") {
    MatchStateMachine m{MatchSettings{}};
    MatchScore score;
    REQUIRE(m.transition(Phase::Build));
    for (int r = 1; r <= 3; ++r) {
        CHECK(m.round() == r);
        CHECK_FALSE(canJoin(m.phase()));
        REQUIRE(m.transition(Phase::Run));
        REQUIRE(m.transition(Phase::Edit));
        REQUIRE(m.transition(Phase::Premiere));
        CHECK(canJoin(m.phase()));
        score.addRatings({{1, 1.5}});
        REQUIRE(m.transition(r < 3 ? Phase::Build : Phase::Podium));
    }
    CHECK(m.phase() == Phase::Podium);
    CHECK(score.total(1) == 4.5);
    CHECK(score.roundsPlayed() == 0);
}

TEST_CASE("podium returns to the lobby with scores reset and players kept") {
    MatchStateMachine m{MatchSettings{}};
    MatchScore score;
    const std::vector<int> players{3, 5};
    CHECK_FALSE(returnToLobby(m, score, players));
    for (int r = 1; r <= m.settings().rounds; ++r) {
        REQUIRE(m.transition(Phase::Build));
        REQUIRE(m.transition(Phase::Run));
        RoundInput in;
        in.players = {3, 5};
        in.finishers = {3};
        score.addRound(in);
        REQUIRE(m.transition(Phase::Edit));
        REQUIRE(m.transition(Phase::Premiere));
    }
    REQUIRE(m.transition(Phase::Podium));
    REQUIRE(score.total(3) > 0.0);

    CHECK(returnToLobby(m, score, players));
    CHECK(m.phase() == Phase::Lobby);
    CHECK(m.round() == 0);
    CHECK(score.roundsPlayed() == 0);
    CHECK(score.totals().size() == players.size());
    CHECK(score.total(3) == 0.0);
    CHECK(score.total(5) == 0.0);
    CHECK(score.podium() == players);
    CHECK(m.transition(Phase::Build)); // the same room starts again
    CHECK(m.round() == 1);
}
