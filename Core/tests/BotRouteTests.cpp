#include "doctest/doctest.h"
#include "tta/BotBrain.h"
#include "tta/BotRoute.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("bot route walks marker to marker and finishes") {
    BotRoute route({{4, 0, 0}, {8, 0, 0}, {12, 0, 0}});
    RouteStep s = route.step({{0, 0, 0}, true, false, 0.1});
    CHECK(s.dirX == doctest::Approx(1.0));
    CHECK(s.dirY == doctest::Approx(0.0));
    CHECK_FALSE(s.jump);
    CHECK(route.target() == 0);
    route.step({{3.5, 0, 0}, true, false, 0.1});
    CHECK(route.target() == 1);
    s = route.step({{11.5, 0.2, 0}, true, false, 0.1});
    CHECK(s.done);
    CHECK(s.dirX == 0.0);
    route.restart();
    CHECK(route.target() == 0);
}

TEST_CASE("bot route does not turn back for a marker it ran past") {
    BotRoute route({{4, 0, 0}, {8, 0, 0}, {12, 0, 0}});
    route.step({{3.5, 0, 0}, true, false, 0.1});
    REQUIRE(route.target() == 1);
    const RouteStep s = route.step({{9.5, 2.0, 0}, true, false, 0.1}); // 2 m wide of marker 1
    CHECK(route.target() == 2);
    CHECK(s.dirX > 0.0);
}

TEST_CASE("bot route jumps at gaps and step-ups unless failing on purpose") {
    BotRoute route({{4, 0, 0}, {8, 0, 1.0}});
    CHECK(route.step({{0, 0, 0}, false, false, 0.1}).jump);
    CHECK_FALSE(route.step({{0, 0, 0}, false, true, 0.1}).jump);
    CHECK(route.step({{4, 0, 0}, true, false, 0.1}).jump); // next point 1 m up
}

TEST_CASE("bot route asks for a respawn when stuck, not while progressing") {
    BotRoute route({{10, 0, 0}});
    bool respawn = false;
    for (int i = 0; i < 29; ++i) respawn |= route.step({{0, 0, 0}, true, false, 0.1}).respawn;
    CHECK_FALSE(respawn);
    CHECK(route.step({{0, 0, 0}, true, false, 0.2}).respawn);
    BotRoute moving({{100, 0, 0}});
    for (int i = 0; i < 100; ++i) CHECK_FALSE(moving.step({{i * 0.6, 0, 0}, true, false, 0.1}).respawn);
}

TEST_CASE("bot route rejects bad input") {
    CHECK_THROWS_AS(BotRoute({}), std::invalid_argument);
    BotRoute route({{1, 0, 0}});
    CHECK_THROWS_AS(route.step({{0, 0, 0}, true, false, -1}), std::invalid_argument);
}

TEST_CASE("bot vote skips own cuts and fail rolls follow the plan") {
    BotBrain bot(2, 7);
    CHECK_FALSE(bot.vote(Cut{2, 3}).has_value());
    CHECK_FALSE(bot.vote(Cut{1, 2}).has_value());
    const auto v = bot.vote(Cut{1, 3});
    REQUIRE(v.has_value());
    CHECK(v->first >= kMinRating);
    CHECK(v->second <= kMaxRating);
    const RunnerPlan never{RunnerSkill::Pro, 0.0};
    const RunnerPlan always{RunnerSkill::Clumsy, 1.0};
    for (int i = 0; i < 50; ++i) {
        CHECK_FALSE(bot.rollFail(never));
        CHECK(bot.rollFail(always));
    }
}
