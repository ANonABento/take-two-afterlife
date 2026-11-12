#include "tta/MenuFlow.h"

#include <algorithm>
#include <cstdlib>

namespace tta {

namespace {
bool canOpen(MenuScreen from, MenuScreen to) {
    switch (to) {
        case MenuScreen::Title: return false;
        case MenuScreen::Lobby: return from == MenuScreen::Title || from == MenuScreen::Join;
        case MenuScreen::Settings: return from == MenuScreen::Title || from == MenuScreen::Lobby;
        default: return from == MenuScreen::Title;
    }
}

int wrap(int value, int size) { return ((value % size) + size) % size; }
}  // namespace

bool MenuFlow::open(MenuScreen screen) {
    if (!canOpen(current(), screen)) return false;
    // Join is replaced by the Lobby it led to, so Back from the Lobby goes to Title.
    if (current() == MenuScreen::Join && screen == MenuScreen::Lobby) stack_.pop_back();
    stack_.push_back(screen);
    return true;
}

bool MenuFlow::back() {
    if (stack_.size() == 1) return false;
    stack_.pop_back();
    return true;
}

PlayerLook cycleLook(PlayerLook look, LookPart part, int step) {
    switch (part) {
        case LookPart::Colour: look.colour = wrap(look.colour + step, static_cast<int>(kPlayerColours.size())); break;
        case LookPart::Hat: look.hat = wrap(look.hat + step, static_cast<int>(kHats.size())); break;
        case LookPart::Face: look.face = wrap(look.face + step, static_cast<int>(kFaces.size())); break;
    }
    return look;
}

ClientSettings clampClientSettings(ClientSettings s) {
    for (int* v : {&s.master, &s.music, &s.vo, &s.sfx}) *v = std::clamp(*v, 0, 100);
    s.sensitivity = std::clamp(s.sensitivity, 10, 500);
    return s;
}

std::string startBlockedReason(int players, int readyPlayers) {
    if (players < 2) return "Need at least 2 players";
    if (readyPlayers < players) return "Waiting for everyone to be ready";
    return {};
}

float micLevel(const std::int16_t* samples, std::size_t count) {
    int peak = 0;
    for (std::size_t i = 0; i < count; ++i) peak = std::max(peak, std::abs(static_cast<int>(samples[i])));
    return std::min(1.f, static_cast<float>(peak) / 32767.f);
}

}  // namespace tta
