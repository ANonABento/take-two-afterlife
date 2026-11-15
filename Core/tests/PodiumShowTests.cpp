#include "doctest/doctest.h"
#include "tta/PodiumShow.h"

#include <stdexcept>
#include <vector>

using namespace tta;

TEST_CASE("after the podium: travel, statues, hold, then lobby, in that order") {
    PodiumShow show({4, 2, 9, 1}, false, 6.0);
    std::vector<PodiumStep> seen{show.step()};
    show.arrived();
    seen.push_back(show.step());
    show.statuesPlaced();
    seen.push_back(show.step());
    show.tick(5.9);
    CHECK(show.step() == PodiumStep::Hold);
    show.tick(0.1);
    seen.push_back(show.step());
    CHECK(seen == std::vector<PodiumStep>{PodiumStep::Travel, PodiumStep::Statues, PodiumStep::Hold, PodiumStep::Lobby});
}

TEST_CASE("already in Heaven skips the travel") {
    PodiumShow show({1}, true, 0.0);
    CHECK(show.step() == PodiumStep::Statues);
    CHECK_THROWS_AS(show.arrived(), std::logic_error);
}

TEST_CASE("steps can't be skipped") {
    PodiumShow show({1, 2}, false, 1.0);
    CHECK_THROWS_AS(show.statuesPlaced(), std::logic_error);
    show.tick(10.0);
    CHECK(show.step() == PodiumStep::Travel);
    CHECK_THROWS_AS(PodiumShow({1}, true, -1.0), std::invalid_argument);
}

TEST_CASE("statues are the top 3, winner first") {
    CHECK(PodiumShow({4, 2, 9, 1}, true, 1.0).statues() == std::vector<int>{4, 2, 9});
    CHECK(PodiumShow({5, 3}, true, 1.0).statues() == std::vector<int>{5, 3});
}

TEST_CASE("statues stand through the lobby and go when the next match builds") {
    CHECK(statuesStand(Phase::Podium));
    CHECK(statuesStand(Phase::Lobby));
    CHECK_FALSE(statuesStand(Phase::Build));
    CHECK_FALSE(statuesStand(Phase::Premiere));
}
