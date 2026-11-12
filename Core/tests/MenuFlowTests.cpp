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
