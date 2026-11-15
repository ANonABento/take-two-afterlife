#include "doctest/doctest.h"
#include "tta/Lawnmower.h"
#include "tta/Placement.h"

#include <stdexcept>
#include <string>

using namespace tta;

namespace {
// A lawnmower at x = 10 m, y = 0, facing +X, default settings (4 m/s, 4 m).
Placement mower() {
    Layout layout;
    REQUIRE(layout.placeTrap(1, TrapType::Lawnmower, 10, 0, 0) == PlaceResult::Ok);
    return layout.all()[0];
}
}

TEST_CASE("lawnmower defaults: 4 m/s for 4 m, 3 x 1.5 m zone") {
    const TrapSpec& s = trapSpec(TrapType::Lawnmower);
    CHECK(s.settings[0].defaultValue == 4);
    CHECK(s.settings[1].defaultValue == 4);
    CHECK(kLawnmowerZoneDepthMeters == 3.0);
    CHECK(kLawnmowerZoneWidthMeters == 1.5);
    CHECK_THROWS_AS(Lawnmower(Placement{}), std::invalid_argument); // a hammer
}

TEST_CASE("lawnmower triggers only when a runner enters the zone in front") {
    RunRecorder rec({0, 1, 2}, 30.0);
    Lawnmower m(mower());
    CHECK_FALSE(m.inTriggerZone({0, 9.0, 0.0}));  // behind
    CHECK_FALSE(m.inTriggerZone({0, 12.0, 1.0})); // beside the zone
    CHECK_FALSE(m.inTriggerZone({0, 14.0, 0.0})); // past 3 m
    CHECK(m.inTriggerZone({0, 13.0, 0.7}));
    m.update(1.0, {{0, 9.0, 0.0}, {1, 12.0, 1.0}}, rec);
    CHECK(m.state() == LawnmowerState::Idle);
    m.update(2.0, {{0, 13.0, 0.0}}, rec);
    CHECK(m.state() == LawnmowerState::Driving);
    CHECK(m.offsetMeters() == 0.0);
}

TEST_CASE("lawnmower zone turns with the trap's rotation") {
    Placement p = mower();
    p.rotation = 2; // 90 degrees: facing +Y
    Lawnmower m(p);
    CHECK(m.inTriggerZone({0, 10.0, 2.0}));
    CHECK_FALSE(m.inTriggerZone({0, 12.0, 0.0}));
}

TEST_CASE("lawnmower drives 4 m at 4 m/s, then is spent for the round") {
    RunRecorder rec({0}, 30.0);
    Lawnmower m(mower());
    m.update(2.0, {{0, 12.0, 0.0}}, rec);
    m.update(2.5, {}, rec);
    CHECK(m.offsetMeters() == doctest::Approx(2.0));
    CHECK(m.state() == LawnmowerState::Driving);
    m.update(3.0, {}, rec);
    CHECK(m.offsetMeters() == doctest::Approx(4.0));
    CHECK(m.state() == LawnmowerState::Spent);
    m.update(9.0, {{0, 12.0, 0.0}}, rec); // one-shot: a runner in the zone does nothing now
    CHECK(m.state() == LawnmowerState::Spent);
    CHECK(m.offsetMeters() == doctest::Approx(4.0));
    CHECK(m.driveOffsetMeters(60000) == 4.0);
    CHECK_THROWS_AS(m.update(8.0, {}, rec), std::invalid_argument);
}

TEST_CASE("lawnmower hits runners it overlaps while moving, once each, through the hit path") {
    RunRecorder rec({0, 1, 2, 3}, 30.0);
    const Placement p = mower();
    Lawnmower m(p);
    rec.advance(2.0);
    m.update(2.0, {{0, 12.0, 0.0}}, rec); // 0 triggers it, still 2 m ahead
    CHECK(rec.replay().events().empty());
    rec.advance(0.5);
    // 2 m along: 0 and 1 overlap, 2 is to the side, 3 is behind.
    const std::vector<RunnerSpot> runners{{0, 12.0, 0.0}, {1, 12.3, 0.2}, {2, 12.0, 1.0}, {3, 9.0, 0.0}};
    m.update(2.5, runners, rec);
    m.update(2.6, runners, rec);
    const auto& ev = rec.replay().events();
    REQUIRE(ev.size() == 2);
    CHECK(ev[0].type == ReplayEventType::Hit);
    CHECK(ev[0].victim == 0);
    CHECK(ev[1].victim == 1);
    CHECK(ev[0].timeMs == 2500);
    CHECK(ev[0].trapId == p.id);
    CHECK(ev[0].trapOwner == 1);
    rec.advance(0.4);
    m.update(3.0, {}, rec);
    CHECK(m.state() == LawnmowerState::Spent);
    rec.advance(5.0);
    m.update(8.0, {{2, 14.0, 0.0}}, rec); // spent at its end point: no hit
    CHECK(rec.replay().events().size() == 2);
}

TEST_CASE("lawnmower survives the layout JSON round-trip") {
    Layout layout;
    int id = 0;
    REQUIRE(layout.placeTrap(1, TrapType::Lawnmower, 10, 0.5, 90, &id) == PlaceResult::Ok);
    REQUIRE(layout.setTrapSetting(id, 0, 6) == PlaceResult::Ok);
    CHECK(layout.setTrapSetting(id, 1, 9) == PlaceResult::BadSetting);
    const Layout back = parseLayout(toJson(layout));
    REQUIRE(back.all().size() == 1);
    const Placement& p = back.all()[0];
    CHECK(p.trap == TrapType::Lawnmower);
    CHECK(p.cellX == 20);
    CHECK(p.cellY == 1);
    CHECK(p.rotation == 2);
    CHECK(p.settings == std::array<int, 2>{6, 4});
    CHECK(toJson(back) == toJson(layout));
}

TEST_CASE("cannon is kept in code but rejected from placement and saved layouts") {
    CHECK_FALSE(trapPlaceable(TrapType::Cannon));
    CHECK(trapPlaceable(TrapType::Lawnmower));
    Layout layout;
    CHECK(layout.placeTrap(1, TrapType::Cannon, 10, 0, 0) == PlaceResult::BadItem);
    CHECK(layout.canPlace(1, false, TrapType::Cannon, 10, 0) == PlaceResult::BadItem);
    CHECK(layout.all().empty());
    CHECK(trapSpec(TrapType::Cannon).cost == 25);
    REQUIRE(layout.placeTrap(1, TrapType::Hammer, 10, 0, 0) == PlaceResult::Ok);
    std::string json = toJson(layout);
    json.replace(json.find("hammer"), 6, "cannon");
    CHECK_THROWS_AS(parseLayout(json), std::invalid_argument);
}
