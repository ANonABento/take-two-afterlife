#include "doctest/doctest.h"
#include "tta/Premiere.h"

#include <set>
#include <stdexcept>

using namespace tta;

namespace {
constexpr MatchSettings kDefaults{};
constexpr std::uint32_t kIntroMs = kDefaults.introSeconds * 1000u;
constexpr std::uint32_t kRatingMs = kDefaults.ratingSeconds * 1000u;
constexpr std::uint32_t kRevealMs = kDefaults.revealSeconds * 1000u;

std::vector<PremiereCut> fourCuts() {
    return {{10, {0, 1}, 1, 5000}, {11, {1, 2}, 2, 6000}, {12, {2, 3}, 3, 7000}, {13, {3, 0}, 0, 8000}};
}
}

TEST_CASE("premiere order is a seeded permutation") {
    Premiere a(fourCuts(), 7, false), b(fourCuts(), 7, false);
    std::set<int> ids;
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(a.order()[i].cutId == b.order()[i].cutId);
        ids.insert(a.order()[i].cutId);
    }
    CHECK(ids.size() == 4);
    bool differs = false;
    for (std::uint32_t seed = 0; seed < 20 && !differs; ++seed) {
        Premiere c(fourCuts(), seed, false);
        for (std::size_t i = 0; i < 4; ++i) differs |= c.order()[i].cutId != a.order()[i].cutId;
    }
    CHECK(differs);
}

TEST_CASE("each cut runs intro, playback, 10 s rating, reveal; final round ends on podium") {
    Premiere p(fourCuts(), 3, true);
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(p.index() == i);
        CHECK(p.step() == PremiereStep::Intro);
        p.advance(kIntroMs);
        CHECK(p.step() == PremiereStep::Playback);
        CHECK_THROWS_AS(p.rate(9, 3, 3), std::logic_error);
        p.advance(p.current().durationMs - 1);
        CHECK(p.step() == PremiereStep::Playback);
        p.advance(1);
        CHECK(p.step() == PremiereStep::Rating);
        CHECK(p.stepLengthMs() == 10000);
        CHECK_THROWS_AS(p.rate(p.current().cut.editor, 5, 5), std::invalid_argument);
        p.rate(9, 4, 2);
        p.advance(kRatingMs);
        CHECK(p.step() == PremiereStep::Reveal);
        CHECK(p.ratings().averages(p.current().cutId).funny == doctest::Approx(4.0));
        p.advance(kRevealMs);
    }
    CHECK(p.step() == PremiereStep::Podium);
    p.advance(100000);
    CHECK(p.step() == PremiereStep::Podium);
    CHECK(p.ratings().ratingPoints().at(0) == doctest::Approx(6.0));
}

TEST_CASE("non-final round ends Done; a big tick crosses steps; bad lists throw") {
    Premiere p({{1, {0, 1}, 1, 1000}}, 0, false);
    p.advance(kIntroMs + 1000 + kRatingMs + kRevealMs);
    CHECK(p.step() == PremiereStep::Done);
    CHECK(p.ratings().averages(1).votes == 0);
    CHECK_THROWS_AS(p.ratings().averages(2), std::invalid_argument);
    CHECK_THROWS_AS(Premiere({}, 0, false), std::invalid_argument);
    CHECK_THROWS_AS(Premiere({{1, {0, 1}, 1, 10}, {1, {1, 2}, 2, 10}}, 0, false), std::invalid_argument);
}

TEST_CASE("custom timings change the premiere step lengths") {
    MatchSettings timings;
    timings.introSeconds = 2;
    timings.ratingSeconds = 7;
    timings.revealSeconds = 3;
    Premiere p({{1, {0, 1}, 1, 1000}}, 0, false, timings);
    CHECK(p.stepLengthMs() == 2000);
    p.advance(2000);
    CHECK(p.step() == PremiereStep::Playback);
    p.advance(1000);
    CHECK(p.stepLengthMs() == 7000);
    p.advance(7000);
    CHECK(p.stepLengthMs() == 3000);
    p.advance(2999);
    CHECK(p.step() == PremiereStep::Reveal);
    p.advance(1);
    CHECK(p.step() == PremiereStep::Done);
}
