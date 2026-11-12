#include "doctest/doctest.h"
#include "tta/Scoring.h"

#include <stdexcept>

using namespace tta;

namespace {
RoundInput fourPlayers() {
    RoundInput r;
    r.players = {1, 2, 3, 4};
    return r;
}
}

TEST_CASE("reaching the gate scores +1") {
    RoundInput r = fourPlayers();
    r.finishers = {1, 3};
    auto pts = scoreRound(r);
    CHECK(pts[1] == 1.0);
    CHECK(pts[2] == 0.0);
    CHECK(pts[3] == 1.0);
    CHECK(pts.size() == 4);
}

TEST_CASE("a wipeout pays the trap cost to its owner") {
    RoundInput r = fourPlayers();
    r.finishers = {1};
    r.wipeouts = {{2, 1, TrapType::Hammer}, {3, 4, TrapType::Banana}, {3, 4, TrapType::Banana}};
    auto pts = scoreRound(r);
    CHECK(pts[1] == 1.0 + trapSpec(TrapType::Hammer).cost);
    CHECK(pts[4] == 2.0 * trapSpec(TrapType::Banana).cost);
    CHECK(pts[2] == 0.0);
    CHECK_FALSE(trapPointsVoided(r));
}

TEST_CASE("trap points are at least 1") {
    for (const TrapSpec& s : trapCatalogue()) {
        RoundInput r = fourPlayers();
        r.finishers = {1};
        r.wipeouts = {{2, 3, s.type}};
        CHECK(scoreRound(r)[3] >= kMinTrapPoints);
    }
}

TEST_CASE("own-trap wipeouts score nothing") {
    RoundInput r = fourPlayers();
    r.finishers = {1};
    r.wipeouts = {{2, 2, TrapType::Spinner}};
    CHECK(scoreRound(r)[2] == 0.0);
}

TEST_CASE("all-finish voids trap points but keeps gate points") {
    RoundInput r = fourPlayers();
    r.finishers = {1, 2, 3, 4};
    r.wipeouts = {{2, 1, TrapType::Hammer}};
    CHECK(trapPointsVoided(r));
    auto pts = scoreRound(r);
    for (int p : r.players) CHECK(pts[p] == 1.0);
}

TEST_CASE("none-finish voids trap points") {
    RoundInput r = fourPlayers();
    r.wipeouts = {{2, 1, TrapType::Cannon}, {3, 4, TrapType::Trapdoor}};
    CHECK(trapPointsVoided(r));
    auto pts = scoreRound(r);
    for (int p : r.players) CHECK(pts[p] == 0.0);
}

TEST_CASE("rating averages are added") {
    RoundInput r = fourPlayers();
    r.finishers = {1, 2, 3, 4};
    r.ratingPoints = {{2, 3.5}, {4, 1.25}};
    auto pts = scoreRound(r);
    CHECK(pts[2] == 4.5);
    CHECK(pts[4] == 2.25);
}

TEST_CASE("unknown players are rejected") {
    RoundInput r = fourPlayers();
    r.finishers = {9};
    CHECK_THROWS_AS(scoreRound(r), std::invalid_argument);
    r = fourPlayers();
    r.wipeouts = {{1, 9, TrapType::Hammer}};
    CHECK_THROWS_AS(scoreRound(r), std::invalid_argument);
    r = fourPlayers();
    r.ratingPoints = {{7, 2.0}};
    CHECK_THROWS_AS(scoreRound(r), std::invalid_argument);
}

TEST_CASE("match totals accumulate and crown a podium") {
    MatchScore m;
    RoundInput a = fourPlayers();
    a.finishers = {1};
    a.wipeouts = {{2, 3, TrapType::Hammer}};
    auto ra = m.addRound(a);
    CHECK(ra[3] == trapSpec(TrapType::Hammer).cost);

    RoundInput b = fourPlayers();
    b.finishers = {1, 2, 3, 4};
    b.wipeouts = {{1, 2, TrapType::PusherWall}};
    m.addRound(b);

    CHECK(m.roundsPlayed() == 2);
    CHECK(m.total(1) == 2.0);
    CHECK(m.total(3) == 1.0 + trapSpec(TrapType::Hammer).cost);
    CHECK(m.total(2) == 1.0);
    CHECK(m.total(4) == 1.0);
    CHECK(m.total(99) == 0.0);
    CHECK(m.podium() == std::vector<int>{3, 1, 2, 4});
}
