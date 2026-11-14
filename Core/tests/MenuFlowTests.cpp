#include "doctest/doctest.h"
#include "tta/MenuFlow.h"

using namespace tta;

TEST_CASE("menu stack follows the title -> join -> lobby flow") {
    MenuFlow flow;
    CHECK(flow.current() == MenuScreen::Title);
    CHECK_FALSE(flow.back());
    CHECK_FALSE(flow.open(MenuScreen::Title));
    CHECK(flow.open(MenuScreen::Join));
    CHECK_FALSE(flow.open(MenuScreen::Reels));
    CHECK(flow.open(MenuScreen::Lobby));
    CHECK(flow.open(MenuScreen::Settings));
    CHECK(flow.back());
    CHECK(flow.current() == MenuScreen::Lobby);
    CHECK(flow.back());
    CHECK(flow.current() == MenuScreen::Title);
    CHECK(flow.open(MenuScreen::HowToPlay));
    CHECK_FALSE(flow.open(MenuScreen::Lobby));
}

TEST_CASE("look carousels wrap both ways") {
    PlayerLook look;
    look = cycleLook(look, LookPart::Colour, -1);
    CHECK(look.colour == 3);
    look = cycleLook(look, LookPart::Hat, 7);
    CHECK(look.hat == 1);
    look = cycleLook(look, LookPart::Face, 5);
    CHECK(look.face == 0);
}

TEST_CASE("settings clamp, start gate and mic meter") {
    ClientSettings s;
    s.master = 150;
    s.sfx = -3;
    s.sensitivity = 1;
    s = clampClientSettings(s);
    CHECK(s.master == 100);
    CHECK(s.sfx == 0);
    CHECK(s.sensitivity == 10);
    CHECK(startBlockedReason(1, 1) == "Need at least 2 players");
    CHECK(startBlockedReason(3, 2) == "Waiting for everyone to be ready");
    CHECK(startBlockedReason(2, 2).empty());
    const std::int16_t buf[] = {0, -32768, 100};
    CHECK(micLevel(buf, 3) == doctest::Approx(1.f));
    CHECK(micLevel(buf, 1) == 0.f);
}

TEST_CASE("resolution picker list is deduped, largest first, and finds the current size") {
    const auto choices = resolutionChoices({{1280, 720}, {1920, 1080}, {0, 0}, {1920, 1080}, {1920, 1200}, {800, -1}});
    REQUIRE(choices.size() == 3);
    CHECK(choices[0] == Resolution{1920, 1200});
    CHECK(choices[1] == Resolution{1920, 1080});
    CHECK(choices[2] == Resolution{1280, 720});
    CHECK(resolutionIndex(choices, {1920, 1080}) == 1);
    CHECK(resolutionIndex(choices, {1024, 768}) == 0);
    CHECK(resolutionIndex({}, {1920, 1080}) == -1);
}

TEST_CASE("window modes follow Unreal's EWindowMode order") {
    CHECK(static_cast<int>(WindowMode::Fullscreen) == 0);
    CHECK(static_cast<int>(WindowMode::WindowedFullscreen) == 1);
    CHECK(static_cast<int>(WindowMode::Windowed) == 2);
    CHECK(std::string(kWindowModeNames[1]) == "Windowed Fullscreen");
}
