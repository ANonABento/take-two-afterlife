#include "doctest/doctest.h"
#include "tta/ScoreBoard.h"
#include "tta/Scoring.h"

using namespace tta;

TEST_CASE("rank order: highest first, ties by lower id") {
    CHECK(rankOrder({{4, 2.0}, {1, 5.0}, {3, 2.0}, {2, 0.0}}) == std::vector<int>{1, 3, 4, 2});
}

TEST_CASE("score board rows come in new-rank order with old and new ranks") {
    const auto rows = scoreBoard({{1, 3.0}, {2, 2.0}, {3, 1.0}}, {{1, 3.0}, {2, 2.0}, {3, 4.5}});
    REQUIRE(rows.size() == 3);
    CHECK(rows[0].player == 3);
    CHECK(rows[0].fromRank == 2);
    CHECK(rows[0].toRank == 0);
    CHECK(rows[0].from == 1.0);
    CHECK(rows[0].to == 4.5);
    CHECK(rows[1].player == 1);
    CHECK(rows[1].fromRank == 0);
    CHECK(rows[1].toRank == 1);
    CHECK(rows[2].player == 2);
    CHECK(rows[2].toRank == 2);
}

TEST_CASE("score board adds players missing from one side at 0") {
    const auto rows = scoreBoard({{1, 1.0}}, {{2, 2.0}});
    REQUIRE(rows.size() == 2);
    CHECK(rows[0].player == 2);
    CHECK(rows[0].from == 0.0);
    CHECK(rows[1].player == 1);
    CHECK(rows[1].to == 0.0);
}

TEST_CASE("bars slide value and slot from old to new") {
    const ScoreRow row{7, 1.0, 5.0, 2, 0};
    CHECK(scoreBarAt(row, 0.0).value == 1.0);
    CHECK(scoreBarAt(row, 0.0).slot == 2.0);
    CHECK(scoreBarAt(row, 0.5).value == doctest::Approx(3.0));
    CHECK(scoreBarAt(row, 0.5).slot == doctest::Approx(1.0));
    CHECK(scoreBarAt(row, 2.0).value == 5.0);
    CHECK(scoreBarAt(row, 2.0).slot == 0.0);
}
