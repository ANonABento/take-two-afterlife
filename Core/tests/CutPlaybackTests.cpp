#include "tta/CutPlayback.h"

#include <doctest/doctest.h>

#include <stdexcept>

using namespace tta;

namespace {

Clip clip(int camera, std::uint32_t start, std::uint32_t end, double speed = 1.0) {
    Clip c;
    c.camera = camera;
    c.startMs = start;
    c.endMs = end;
    c.speed = speed;
    return c;
}

} // namespace

TEST_CASE("playback: maps cut time to clip, camera and replay time") {
    EditList cut;
    cut.addClip(clip(2, 1000, 2000));      // cut 0..1000
    cut.addClip(clip(5, 4000, 5000, 0.5)); // cut 1000..3000
    cut.setZoom(1, 2.0);

    auto f = cutFrameAt(cut, 500);
    REQUIRE(f);
    CHECK(f->camera == 2);
    CHECK(f->replaySeconds == doctest::Approx(1.5));

    f = cutFrameAt(cut, 2000); // 1000 ms into a half-speed clip = 500 ms of replay
    REQUIRE(f);
    CHECK(f->clip == 1);
    CHECK(f->camera == 5);
    CHECK(f->zoom == 2.0);
    CHECK(f->replaySeconds == doctest::Approx(4.5));

    CHECK(cutFrameAt(cut, 2999));
    CHECK_FALSE(cutFrameAt(cut, cut.durationMs()));
    CHECK_FALSE(cutFrameAt(EditList{}, 0));
}

TEST_CASE("playback: auto-edit plays for exactly its edit-list length") {
    Replay replay({0, 1});
    const std::vector<ReplayPose> poses(2);
    for (int f = 0; f <= 90 * kReplayHz; ++f) replay.addFrame(poses);
    replay.addEvent({ReplayEventType::Hit, 8000, 1, 3, 0});
    replay.addEvent({ReplayEventType::Wipeout, 8600, 1, 3, 0});
    replay.addEvent({ReplayEventType::Smite, 89000, 0, kNoTrap, kNoTrap});
    const EditList cut = autoEdit(replay, {0, 1}, 2);
    std::uint32_t last = 0;
    for (std::uint32_t t = 0; cutFrameAt(cut, t); ++t) last = t;
    CHECK(last + 1 == cut.durationMs());
}

TEST_CASE("playback: a cut survives a save/load round trip") {
    EditList cut;
    cut.addClip(clip(1, 0, 1500, 0.25));
    cut.addClip(clip(3, 2000, 2600));
    cut.setZoom(0, 1.5);
    cut.addOverlay({OverlayType::Caption, 100, 900, "he did NOT see that coming"});
    cut.addOverlay({OverlayType::WastedCard, 6000, 500, ""});

    const EditList back = loadCut(saveCut(cut));
    REQUIRE(back.clips().size() == 2);
    CHECK(back.clips()[0].speed == 0.25);
    CHECK(back.clips()[0].zoom == 1.5);
    CHECK(back.clips()[1].camera == 3);
    REQUIRE(back.overlays().size() == 2);
    CHECK(back.overlays()[0].content == "he did NOT see that coming");
    CHECK(back.overlays()[1].type == OverlayType::WastedCard);
    CHECK(back.durationMs() == cut.durationMs());
}

TEST_CASE("playback: bad cut files are rejected") {
    CHECK_THROWS_AS(loadCut("nope\n"), std::invalid_argument);
    CHECK_THROWS_AS(loadCut("TTACUT 1\nclip 1 0\n"), std::invalid_argument);
    CHECK_THROWS_AS(loadCut("TTACUT 1\nclip 1 0 40000 1 1\n"), std::invalid_argument); // over 30 s cap
    CHECK_THROWS_AS(loadCut("TTACUT 1\nbanana\n"), std::invalid_argument);
    EditList cut;
    cut.addClip(clip(1, 0, 1000));
    cut.addOverlay({OverlayType::Caption, 0, 500, "two\nlines"});
    CHECK_THROWS_AS(saveCut(cut), std::invalid_argument);
}
