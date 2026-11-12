#include "tta/EditList.h"

#include <doctest/doctest.h>

#include <stdexcept>

using namespace tta;

namespace {

Clip clip(int camera, std::uint32_t start, std::uint32_t end) {
    Clip c;
    c.camera = camera;
    c.startMs = start;
    c.endMs = end;
    return c;
}

// Fixture: 2 players, 60 s at 20 Hz, with a hit, a wipeout and a smite.
Replay fixtureReplay() {
    Replay replay({0, 1});
    const std::vector<ReplayPose> poses(2);
    for (int f = 0; f <= 60 * kReplayHz; ++f) {
        replay.addFrame(poses);
    }
    replay.addEvent({ReplayEventType::Hit, 5000, 1, 3, 0});
    replay.addEvent({ReplayEventType::Wipeout, 5800, 1, 3, 0});
    replay.addEvent({ReplayEventType::Smite, 59500, 0, kNoTrap, kNoTrap});
    return replay;
}

} // namespace

TEST_CASE("edit: trim changes the clip's range and duration") {
    EditList cut;
    cut.addClip(clip(1, 0, 4000));
    cut.trim(0, 1000, 2500);
    CHECK(cut.clips()[0].startMs == 1000);
    CHECK(cut.durationMs() == 1500);
    CHECK_THROWS_AS(cut.trim(0, 2000, 2000), std::invalid_argument);
    CHECK(cut.durationMs() == 1500); // rejected edit left it alone
    CHECK_THROWS_AS(cut.trim(3, 0, 10), std::out_of_range);
}

TEST_CASE("edit: reorder moves a clip") {
    EditList cut;
    cut.addClip(clip(1, 0, 1000));
    cut.addClip(clip(2, 0, 1000));
    cut.addClip(clip(3, 0, 1000));
    cut.move(2, 0);
    CHECK(cut.clips()[0].camera == 3);
    CHECK(cut.clips()[1].camera == 1);
    CHECK(cut.clips()[2].camera == 2);
}

TEST_CASE("edit: multi-angle splits a clip and switches camera") {
    EditList cut;
    cut.addClip(clip(1, 1000, 5000));
    cut.split(0, 3000);
    cut.setCamera(1, 7);
    REQUIRE(cut.clips().size() == 2);
    CHECK(cut.clips()[0].endMs == 3000);
    CHECK(cut.clips()[1].startMs == 3000);
    CHECK(cut.clips()[1].camera == 7);
    CHECK(cut.durationMs() == 4000);
    CHECK_THROWS_AS(cut.split(0, 1000), std::invalid_argument);
}

TEST_CASE("edit: slow-mo stretches cut time") {
    EditList cut;
    cut.addClip(clip(1, 0, 2000));
    cut.setSpeed(0, 0.5);
    CHECK(cut.durationMs() == 4000);
    CHECK_THROWS_AS(cut.setSpeed(0, 0.1), std::invalid_argument);
    CHECK_THROWS_AS(cut.setSpeed(0, 2.0), std::invalid_argument);
}

TEST_CASE("edit: zoom is kept within range") {
    EditList cut;
    cut.addClip(clip(1, 0, 2000));
    cut.setZoom(0, 2.5);
    CHECK(cut.clips()[0].zoom == 2.5);
    CHECK_THROWS_AS(cut.setZoom(0, 0.5), std::invalid_argument);
    CHECK_THROWS_AS(cut.setZoom(0, 5.0), std::invalid_argument);
}

TEST_CASE("edit: overlays (sticker, caption, wasted card) must sit inside the cut") {
    EditList cut;
    cut.addClip(clip(1, 0, 3000));
    cut.addOverlay({OverlayType::Sticker, 0, 500, "bonk"});
    cut.addOverlay({OverlayType::Caption, 500, 1000, "he never saw it"});
    cut.addOverlay({OverlayType::WastedCard, 2000, 1000, ""});
    CHECK(cut.overlays().size() == 3);
    CHECK_THROWS_AS(cut.addOverlay({OverlayType::Caption, 2500, 1000, "late"}), std::invalid_argument);
    CHECK_THROWS_AS(cut.addOverlay({OverlayType::Caption, 0, 500, ""}), std::invalid_argument);
    CHECK_THROWS_AS(cut.trim(0, 0, 2000), std::invalid_argument); // would strand the card
    cut.removeOverlay(2);
    cut.trim(0, 0, 2000);
    CHECK(cut.durationMs() == 2000);
}

TEST_CASE("edit: 30 s cap is enforced by every op") {
    EditList cut;
    cut.addClip(clip(1, 0, 20000));
    cut.addClip(clip(2, 0, 10000));
    CHECK(cut.durationMs() == kMaxCutMs);
    CHECK_THROWS_AS(cut.addClip(clip(3, 0, 1)), std::invalid_argument);
    CHECK_THROWS_AS(cut.setSpeed(1, 0.5), std::invalid_argument);
    CHECK_THROWS_AS(cut.trim(1, 0, 10001), std::invalid_argument);
    CHECK(cut.durationMs() == kMaxCutMs);
    CHECK(cut.clips().size() == 2);
}

TEST_CASE("auto-edit: builds a cut from hit events on a fixture replay") {
    const Replay replay = fixtureReplay();
    const EditList cut = autoEdit(replay, {4, 5}, 99);
    REQUIRE(cut.clips().size() == 2);
    // Hit at 5.0 s and wipeout at 5.8 s merge into one slow-mo clip.
    const Clip& first = cut.clips()[0];
    CHECK(first.camera == 4);
    CHECK(first.startMs == 3500);
    CHECK(first.endMs == 7300);
    CHECK(first.speed == 0.5);
    CHECK(first.zoom > 1.0);
    // Smite near the end is clamped to the replay and filmed by the Heaven Cam.
    const Clip& smite = cut.clips()[1];
    CHECK(smite.camera == 99);
    CHECK(smite.startMs == 58000);
    CHECK(smite.endMs == 60000);
    CHECK(cut.durationMs() == 7600 + 4000);
    REQUIRE(cut.overlays().size() == 2);
    CHECK(cut.overlays()[0].type == OverlayType::WastedCard);
    CHECK(cut.overlays()[1].atMs + cut.overlays()[1].durationMs == cut.durationMs());
}

TEST_CASE("auto-edit: respects the 30 s cap and rotates cameras") {
    Replay replay({0, 1});
    const std::vector<ReplayPose> poses(2);
    for (int f = 0; f <= 180 * kReplayHz; ++f) {
        replay.addFrame(poses);
    }
    for (std::uint32_t t = 10000; t < 170000; t += 10000) {
        replay.addEvent({ReplayEventType::Hit, t, 1, 2, 0});
    }
    const EditList cut = autoEdit(replay, {1, 2, 3}, 99);
    CHECK(cut.durationMs() == kMaxCutMs);
    REQUIRE(cut.clips().size() == 10);
    CHECK(cut.clips()[0].camera == 1);
    CHECK(cut.clips()[1].camera == 2);
    CHECK(cut.clips()[3].camera == 1);
}

TEST_CASE("auto-edit: no events gives an empty cut; no cameras throws") {
    Replay replay({0});
    replay.addFrame(std::vector<ReplayPose>(1));
    CHECK(autoEdit(replay, {1}, 99).clips().empty());
    CHECK_THROWS_AS(autoEdit(replay, {}, 99), std::invalid_argument);
}
