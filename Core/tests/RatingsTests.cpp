#include "doctest/doctest.h"
#include "tta/Ratings.h"
#include "tta/Scoring.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("self-rating is rejected for editor and builder") {
    RatingBook book;
    book.openCut(1, Cut{1, 2});
    CHECK_THROWS_AS(book.rate(1, 1, 5, 5), std::invalid_argument);
    CHECK_THROWS_AS(book.rate(1, 2, 5, 5), std::invalid_argument);
    CHECK(book.ratingPoints().empty());
    book.rate(1, 3, 4, 2);
    CHECK(book.ratingPoints().at(1) == doctest::Approx(4.0));
    CHECK(book.ratingPoints().at(2) == doctest::Approx(2.0));
}

TEST_CASE("bad ratings are rejected and change nothing") {
    RatingBook book;
    book.openCut(1, Cut{1, 2});
    CHECK_THROWS_AS(book.rate(9, 3, 3, 3), std::invalid_argument);
    CHECK_THROWS_AS(book.rate(1, 3, 0, 3), std::invalid_argument);
    CHECK_THROWS_AS(book.rate(1, 3, 3, 6), std::invalid_argument);
    CHECK(book.ratingPoints().empty());
    book.rate(1, 3, 3, 3);
    CHECK_THROWS_AS(book.rate(1, 3, 5, 5), std::invalid_argument);
    CHECK_THROWS_AS(book.openCut(1, Cut{3, 4}), std::invalid_argument);
}

TEST_CASE("averages aggregate per player across cuts and roles") {
    RatingBook book;
    book.openCut(1, Cut{1, 2}); // 1 edited, 2 built
    book.openCut(2, Cut{2, 1}); // 2 edited, 1 built
    book.openCut(3, Cut{3, 3}); // unrated
    book.rate(1, 3, 5, 1);
    book.rate(1, 4, 2, 4);
    book.rate(2, 3, 3, 5);
    const auto pts = book.ratingPoints();
    CHECK(pts.at(1) == doctest::Approx(3.5 + 5.0)); // funny avg on cut 1 + evil on cut 2
    CHECK(pts.at(2) == doctest::Approx(2.5 + 3.0));
    CHECK(pts.count(3) == 0);
}

TEST_CASE("rating points feed the match score") {
    RatingBook book;
    book.openCut(1, Cut{1, 2});
    book.rate(1, 3, 4, 3);
    RoundInput round;
    round.players = {1, 2, 3};
    round.finishers = {3};
    round.ratingPoints = book.ratingPoints();
    const auto pts = scoreRound(round);
    CHECK(pts.at(1) == doctest::Approx(4.0));
    CHECK(pts.at(2) == doctest::Approx(3.0));
    CHECK(pts.at(3) == doctest::Approx(1.0));
}
