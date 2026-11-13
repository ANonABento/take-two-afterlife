#include "doctest/doctest.h"
#include "tta/TrapTimeline.h"

#include <limits>
#include <stdexcept>

using namespace tta;

namespace {
std::array<int, 2> defaults(const TrapSpec& s) { return {s.settings[0].defaultValue, s.settings[1].defaultValue}; }
} // namespace

TEST_CASE("every trap is deterministic: same settings and time give the same pose") {
    for (const TrapSpec& spec : trapCatalogue()) {
        CAPTURE(spec.name);
        for (int a = spec.settings[0].min; a <= spec.settings[0].max; ++a)
            for (int b = spec.settings[1].min; b <= spec.settings[1].max; ++b)
                for (double t : {0.0, 0.25, 1.337, 7.9, 123.456})
                    CHECK(pose(spec.type, {a, b}, t) == pose(spec.type, {a, b}, t));
    }
}

TEST_CASE("every trap repeats exactly after its period") {
    for (const TrapSpec& spec : trapCatalogue()) {
        CAPTURE(spec.name);
        for (int a = spec.settings[0].min; a <= spec.settings[0].max; ++a)
            for (int b = spec.settings[1].min; b <= spec.settings[1].max; ++b) {
                const std::int64_t period = trapPeriodMs(spec.type, {a, b});
                const double p = (period == 0 ? 3000 : period) / 1000.0;
                for (double t : {0.0, 0.4, 1.1, 2.75, 9.05}) {
                    CHECK(pose(spec.type, {a, b}, t) == pose(spec.type, {a, b}, t + p));
                    CHECK(pose(spec.type, {a, b}, t) == pose(spec.type, {a, b}, t + 5 * p));
                }
            }
    }
}

TEST_CASE("moving traps actually move within one period") {
    for (const TrapSpec& spec : trapCatalogue()) {
        if (spec.type == TrapType::Banana) continue;
        CAPTURE(spec.name);
        const auto s = defaults(spec);
        const double half = trapPeriodMs(spec.type, s) / 2000.0;
        bool moved = false;
        for (double t = 0.05; t < 2 * half; t += 0.05) moved = moved || !(pose(spec.type, s, 0.0) == pose(spec.type, s, t));
        CHECK(moved);
    }
}

TEST_CASE("settings shape the timeline") {
    CHECK(trapPeriodMs(TrapType::Hammer, {1, 0}) > trapPeriodMs(TrapType::Hammer, {5, 0}));
    CHECK(pose(TrapType::Hammer, {3, 0}, 0.0).angleDeg == doctest::Approx(60.0));
    CHECK(pose(TrapType::Hammer, {3, 1}, 0.0).angleDeg == doctest::Approx(-60.0));
    CHECK(pose(TrapType::Cannon, {3, 3}, 0.5).projectileRadiusMeters > pose(TrapType::Cannon, {3, 1}, 0.5).projectileRadiusMeters);
    CHECK(pose(TrapType::Spinner, {3, 3}, 0.0).extensionMeters > pose(TrapType::Spinner, {3, 1}, 0.0).extensionMeters);
    CHECK(pose(TrapType::PusherWall, {5, 4}, 1.5).extensionMeters > pose(TrapType::PusherWall, {1, 4}, 1.5).extensionMeters);
    CHECK(trapPeriodMs(TrapType::PusherWall, {3, 4}) == 4000);
    // Trapdoor: closed for 1 s + delay, then open for the duration.
    CHECK(pose(TrapType::Trapdoor, {2, 3}, 2.9).openFraction == 0.0);
    CHECK(pose(TrapType::Trapdoor, {2, 3}, 3.0).openFraction == 1.0);
    CHECK(pose(TrapType::Trapdoor, {2, 3}, 5.9).openFraction == 1.0);
    CHECK(pose(TrapType::Trapdoor, {2, 3}, 6.0).openFraction == 0.0);
    CHECK(pose(TrapType::Banana, {4, 3}, 17.0).count == 3);
    CHECK(trapPeriodMs(TrapType::Banana, {4, 3}) == 0);
}

TEST_CASE("bad settings and times are rejected") {
    CHECK_THROWS_AS(pose(TrapType::Hammer, {0, 0}, 1.0), std::invalid_argument);
    CHECK_THROWS_AS(pose(TrapType::Banana, {3, 4}, 1.0), std::invalid_argument);
    // MSVC rejects a literal divide-by-zero. Infinity is the same non-finite time.
    CHECK_THROWS_AS(pose(TrapType::Spinner, {3, 2}, std::numeric_limits<double>::infinity()), std::invalid_argument);
}
