#include "doctest/doctest.h"
#include "tta/BotBrain.h"

#include <tuple>

using namespace tta;

TEST_CASE("bots stay within budget and camera cap for many seeds") {
    for (std::uint64_t seed = 0; seed < 200; ++seed) {
        Layout layout;
        for (int p = 1; p <= 3; ++p) BotBrain(p, seed).build(layout);
        for (int p = 1; p <= 3; ++p) {
            CHECK(layout.spent(p) <= kBudget);
            CHECK(layout.spent(p) > 0);
            CHECK(layout.cameraCount(p) >= 1);
            CHECK(layout.cameraCount(p) <= kCameraCap);
        }
    }
}

TEST_CASE("same seed gives the same layout, run plan and ratings") {
    auto once = [](std::uint64_t seed) {
        Layout layout;
        BotBrain bot(2, seed);
        bot.build(layout);
        RunnerPlan run = bot.planRun();
        RatingBook book;
        book.openCut(1, Cut{1, 3});
        bot.rate(book, {{1, Cut{1, 3}}});
        auto avg = book.averages(1);
        return std::make_tuple(toJson(layout), static_cast<int>(run.skill), run.failChance, avg.funny, avg.evil);
    };
    CHECK(once(42) == once(42));
    CHECK(once(42) != once(43));
}

TEST_CASE("bots never rate their own cuts and rate others 1-5") {
    RatingBook book;
    std::vector<std::pair<int, Cut>> cuts{{1, Cut{1, 2}}, {2, Cut{2, 3}}, {3, Cut{3, 1}}};
    for (const auto& [id, cut] : cuts) book.openCut(id, cut);
    for (int p = 1; p <= 4; ++p) CHECK_NOTHROW(BotBrain(p, 7).rate(book, cuts));
    for (const auto& entry : cuts) {
        auto avg = book.averages(entry.first);
        CHECK(avg.votes == 2); // 4 bots minus the cut's editor and builder
        CHECK(avg.funny >= kMinRating);
        CHECK(avg.funny <= kMaxRating);
        CHECK(avg.evil >= kMinRating);
        CHECK(avg.evil <= kMaxRating);
    }
}

TEST_CASE("run plan skill and fail chance are in range") {
    for (std::uint64_t seed = 0; seed < 100; ++seed) {
        RunnerPlan run = BotBrain(1, seed).planRun();
        CHECK(run.failChance >= 0.05);
        CHECK(run.failChance <= 0.30);
        CHECK(static_cast<int>(run.skill) <= static_cast<int>(RunnerSkill::Pro));
    }
}
