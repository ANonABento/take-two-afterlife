#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tta {

// Front-end menus (plan task 24): Title, Join (code), Lobby, Settings, How to play, Reels.
enum class MenuScreen { Title, Join, Lobby, Settings, HowToPlay, Reels };

// Screen stack. Title is the root; Lobby is reached from Title (host) or Join (code
// accepted); Settings opens from Title or Lobby; Join, How to play and Reels from Title.
class MenuFlow {
public:
    MenuScreen current() const { return stack_.back(); }
    // False (stack unchanged) when the screen can't be opened from here.
    bool open(MenuScreen screen);
    // False on Title.
    bool back();

private:
    std::vector<MenuScreen> stack_{MenuScreen::Title};
};

// Same order and hex as Scripts/toon_spec.py PALETTE (a Python test keeps them equal).
constexpr std::array<const char*, 12> kPaletteHex = {
    "1E1B2E", "F4F1E8", "E5484D", "F2913D", "F5D547", "6CC24A",
    "2BB3A3", "5BA8F5", "3F5BD9", "8E5CD9", "F27BB0", "9A6A43"};
constexpr int kInk = 0, kBone = 1, kRed = 2, kOrange = 3, kYellow = 4, kGrass = 5,
              kTeal = 6, kSky = 7, kBlue = 8, kPurple = 9, kPink = 10;
// Lobby colour choices, P1-P4 defaults (palette indices).
constexpr std::array<int, 4> kPlayerColours = {kRed, kBlue, kGrass, kOrange};
// models/: SM_Hat_* and T_Face_*, alphabetical.
constexpr std::array<const char*, 6> kHats = {"Chef", "Cowboy", "Firefighter", "HardHat", "Pilot", "Police"};
constexpr std::array<const char*, 6> kFaces = {"Angry", "Happy", "Shocked", "Sleepy", "Smug", "Wink"};

enum class LookPart { Colour, Hat, Face };
struct PlayerLook {
    int colour = 0;  // index into kPlayerColours
    int hat = 0;
    int face = 1;
};
// Steps one carousel by `step` (negative = back), wrapping.
PlayerLook cycleLook(PlayerLook look, LookPart part, int step);

// Local (per-player) settings. Volumes 0-100, sensitivity percent 10-500.
struct ClientSettings {
    int master = 80, music = 70, vo = 100, sfx = 90;
    int sensitivity = 100;
    bool subtitles = true;
    bool readCodeAloud = true;
    bool reduceMotion = false;
};
ClientSettings clampClientSettings(ClientSettings settings);

// Empty when START may be pressed; otherwise the tooltip saying why not.
std::string startBlockedReason(int players, int readyPlayers);

// Mic test meter: peak of a 16-bit buffer, 0..1.
float micLevel(const std::int16_t* samples, std::size_t count);

struct HowToCard {
    const char* title;
    const char* line;
};
constexpr std::array<HowToCard, 4> kHowToPlay = {{
    {"BUILD", "Place traps and hide cameras on the course, within your budget."},
    {"RUN", "Race the course to the gate while everyone's traps go off."},
    {"EDIT", "Cut the best wipeouts from your cameras into a 30-second film."},
    {"PREMIERE", "Watch every film and rate it Funny and Evil."},
}};

}  // namespace tta
