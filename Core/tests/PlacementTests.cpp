#include "doctest/doctest.h"
#include "tta/MatchStateMachine.h"
#include "tta/Placement.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("catalogue has 6 traps with 2 settings and a cost") {
    CHECK(trapCatalogue().size() == 6);
    for (const TrapSpec& s : trapCatalogue()) {
        CHECK(s.cost > 0);
        for (const SettingSpec& set : s.settings) CHECK(set.defaultValue >= set.min);
    }
    CHECK(trapSpec(TrapType::Banana).settings[1].max == 3);
}

TEST_CASE("positions snap to 0.5 m and rotation to 45 degrees") {
    CHECK(snapToCell(1.24) == 2);
    CHECK(snapToCell(1.26) == 3);
    CHECK(snapToCell(-0.3) == -1);
    CHECK(snapRotation(50) == 1);
    CHECK(snapRotation(-45) == 7);
    CHECK(snapRotation(360) == 0);
    Layout l;
    int id = 0;
    REQUIRE(l.placeTrap(1, TrapType::Hammer, 10.37, 1.1, 100, &id) == PlaceResult::Ok);
    CHECK(cellToMeters(l.all()[0].cellX) == 10.5);
    CHECK(cellToMeters(l.all()[0].cellY) == 1.0);
    CHECK(l.all()[0].rotation == 2);
}

TEST_CASE("corridor bounds") {
    Layout l;
    CHECK(l.placeTrap(1, TrapType::Banana, -0.5, 0, 0) == PlaceResult::OutOfBounds);
    CHECK(l.placeTrap(1, TrapType::Banana, 60.3, 0, 0) == PlaceResult::OutOfBounds);
    CHECK(l.placeTrap(1, TrapType::Banana, 30, 4.5, 0) == PlaceResult::OutOfBounds);
    CHECK(l.placeCamera(1, CameraType::Fixed, 30, -4.3, 0) == PlaceResult::OutOfBounds);
    CHECK(l.placeTrap(1, TrapType::Banana, 60, -4, 0) == PlaceResult::Ok);
}

TEST_CASE("budget is 100 per owner") {
    Layout l;
    for (int i = 0; i < 4; ++i) CHECK(l.placeTrap(1, TrapType::Cannon, i, 0, 0) == PlaceResult::Ok);
    CHECK(l.remaining(1) == 0);
    CHECK(l.placeTrap(1, TrapType::Banana, 5, 0, 0) == PlaceResult::OverBudget);
    CHECK(l.placeTrap(2, TrapType::Banana, 5, 0, 0) == PlaceResult::Ok);
    CHECK(l.remove(l.all()[0].id));
    CHECK(l.placeTrap(1, TrapType::Hammer, 5, 0, 0) == PlaceResult::Ok);
    CHECK(l.spent(1) == 95);
}

TEST_CASE("cameras are free and capped at 12 per type per owner") {
    Layout l;
    for (int i = 0; i < kCameraCap; ++i)
        CHECK(l.placeCamera(1, static_cast<CameraType>(i % 4), i, 0, 0) == PlaceResult::Ok);
    CHECK(l.spent(1) == 0);
    CHECK(l.cameraCount(1, CameraType::TrapCam) == 3);
    CHECK(l.placeCamera(1, CameraType::Follow, 20, 0, 0) == PlaceResult::Ok);
    CHECK(l.placeCamera(2, CameraType::Follow, 20, 0, 0) == PlaceResult::Ok);
}

TEST_CASE("the camera cap counts only cameras of the type being placed") {
    Layout l;
    for (int i = 0; i < kCameraCap; ++i) REQUIRE(l.placeCamera(1, CameraType::Pan, i, 0, 0) == PlaceResult::Ok);
    CHECK(l.placeCamera(1, CameraType::Pan, 20, 0, 0) == PlaceResult::CameraCapReached);
    CHECK(l.cameraCount(1, CameraType::Pan) == kCameraCap);
    CHECK(l.placeCamera(1, CameraType::Fixed, 20, 0, 0) == PlaceResult::Ok);
}

TEST_CASE("trap settings are range checked") {
    Layout l;
    int id = 0;
    REQUIRE(l.placeTrap(1, TrapType::Banana, 1, 0, 0, &id) == PlaceResult::Ok);
    CHECK(l.setTrapSetting(id, 1, 3) == PlaceResult::Ok);
    CHECK(l.setTrapSetting(id, 1, 4) == PlaceResult::BadSetting);
    CHECK(l.setTrapSetting(id, 2, 1) == PlaceResult::BadSetting);
}

TEST_CASE("placements are hidden from other owners until reveal") {
    Layout l;
    l.placeTrap(1, TrapType::Spinner, 1, 0, 0);
    l.placeCamera(2, CameraType::Pan, 2, 0, 0);
    CHECK(l.visibleTo(1).size() == 1);
    CHECK(l.visibleTo(2).size() == 1);
    l.revealAll();
    CHECK(l.visibleTo(1).size() == 2);
}

TEST_CASE("JSON round-trip") {
    Layout l;
    int id = 0;
    l.placeTrap(1, TrapType::Trapdoor, 12.5, -3.5, 135, &id);
    l.setTrapSetting(id, 0, 4);
    l.placeCamera(1, CameraType::TrapCam, 0, 4, 270);
    l.revealAll();
    l.placeTrap(2, TrapType::PusherWall, 59.5, 0, 0);
    const std::string json = toJson(l);
    const Layout back = parseLayout(json);
    CHECK(toJson(back) == json);
    REQUIRE(back.all().size() == 3);
    CHECK(back.all()[0].settings[0] == 4);
    CHECK_FALSE(back.all()[1].hidden);
    CHECK(back.all()[2].hidden);
    CHECK(toJson(parseLayout(toJson(Layout()))) == toJson(Layout()));
}

TEST_CASE("bad layout JSON is rejected") {
    CHECK_THROWS_AS(parseLayout("{\"placements\": [}"), std::invalid_argument);
    CHECK_THROWS_AS(parseLayout("{\"placements\": [{\"id\": 1, \"owner\": 1, \"kind\": \"trap\", \"type\": \"piano\", \"cellX\": 0, \"cellY\": 0, \"rotation\": 0, \"setting1\": 1, \"setting2\": 1, \"hidden\": true}]}"), std::invalid_argument);
    CHECK_THROWS_AS(parseLayout("{\"placements\": [{\"id\": 1, \"owner\": 1, \"kind\": \"camera\", \"type\": \"pan\", \"cellX\": 999, \"cellY\": 0, \"rotation\": 0, \"hidden\": true}]}"), std::invalid_argument);
}

TEST_CASE("canPlace previews the place rules without placing") {
    Layout l;
    CHECK(l.canPlace(1, false, TrapType::Cannon, 10.0, 0.0) == PlaceResult::Ok);
    CHECK(l.canPlace(1, false, TrapType::Cannon, -1.0, 0.0) == PlaceResult::OutOfBounds);
    CHECK(l.canPlace(1, true, TrapType::Hammer, 10.0, 4.3) == PlaceResult::OutOfBounds);
    for (int i = 0; i < 4; ++i) REQUIRE(l.placeTrap(1, TrapType::Cannon, 5.0 * i, 0.0, 0.0) == PlaceResult::Ok);
    CHECK(l.canPlace(1, false, TrapType::Banana, 30.0, 0.0) == PlaceResult::OverBudget);
    CHECK(l.canPlace(2, false, TrapType::Banana, 30.0, 0.0) == PlaceResult::Ok);
    for (int i = 0; i < kCameraCap; ++i) REQUIRE(l.placeCamera(1, CameraType::Pan, i, 1.0, 0.0) == PlaceResult::Ok);
    CHECK(l.canPlace(1, true, TrapType::Hammer, 30.0, 0.0, CameraType::Pan) == PlaceResult::CameraCapReached);
    CHECK(l.canPlace(1, true, TrapType::Hammer, 30.0, 0.0, CameraType::Fixed) == PlaceResult::Ok);
    CHECK(l.all().size() == 4 + kCameraCap);
}

TEST_CASE("placements stay between rounds and the budget resets each Build") {
    MatchStateMachine m(parseMatchSettings("{\"rounds\": 3}"));
    Layout layout;
    auto build = [&] { REQUIRE(m.transition(Phase::Build)); layout.beginBuild(m.round()); };
    auto finish = [&] {
        REQUIRE(m.transition(Phase::Run));
        layout.revealAll();
        REQUIRE(m.transition(Phase::Edit));
        REQUIRE(m.transition(Phase::Premiere));
    };
    const int bananas = kBudget / trapSpec(TrapType::Banana).cost;

    build();
    int cannon = 0;
    CHECK(layout.placeTrap(1, TrapType::Cannon, 10, 0, 0, &cannon) == PlaceResult::Ok);
    CHECK(layout.placeCamera(1, CameraType::Fixed, 5, 0, 0) == PlaceResult::Ok);
    CHECK(layout.spent(1) == trapSpec(TrapType::Cannon).cost);
    finish();

    build();
    CHECK(m.round() == 2);
    REQUIRE(layout.all().size() == 2);
    CHECK(layout.all()[0].id == cannon);
    CHECK(layout.all()[0].round == 1);
    CHECK(layout.all()[1].isCamera);
    CHECK(layout.remaining(1) == kBudget);
    CHECK(layout.remaining(2) == kBudget);
    // Round 1's cannon is paid for: a full budget of new traps still fits.
    for (int i = 0; i < bananas; ++i)
        CHECK(layout.placeTrap(1, TrapType::Banana, 20 + i, 1, 0) == PlaceResult::Ok);
    CHECK(layout.remaining(1) == 0);
    CHECK(layout.placeTrap(1, TrapType::Banana, 50, 0, 0) == PlaceResult::OverBudget);
    finish();

    build();
    CHECK(m.round() == 3);
    CHECK(layout.remaining(1) == kBudget);
    CHECK(layout.all().size() == static_cast<size_t>(2 + bananas));
    finish();

    CHECK_FALSE(m.canTransition(Phase::Build));
    CHECK(m.transition(Phase::Podium));
}

TEST_CASE("layout JSON keeps each placement's round and the current round") {
    Layout layout;
    layout.beginBuild(1);
    REQUIRE(layout.placeTrap(1, TrapType::Cannon, 10, 0, 0) == PlaceResult::Ok);
    layout.beginBuild(2);
    REQUIRE(layout.placeTrap(1, TrapType::Hammer, 12, 0, 0) == PlaceResult::Ok);
    const Layout back = parseLayout(toJson(layout));
    CHECK(back.round() == 2);
    REQUIRE(back.all().size() == 2);
    CHECK(back.all()[0].round == 1);
    CHECK(back.all()[1].round == 2);
    CHECK(back.spent(1) == layout.spent(1));
}
