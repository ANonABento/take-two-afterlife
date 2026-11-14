#pragma once

#include <string>

namespace tta {

// Every phase length in one place. Unreal fills it from DefaultGame.ini
// ([/Script/TakeTwoAfterlife.TTAMatchSettings]); the host can save JSON overrides.
struct MatchSettings {
    int rounds = 3;
    int buildSeconds = 120;
    int runSeconds = 120;
    int editSeconds = 90;
    int introSeconds = 4;   // premiere: narrator names director + victim
    int ratingSeconds = 10; // premiere: everyone rates Funny + Evil
    int revealSeconds = 4;  // premiere: score reveal
};

// Parses a flat JSON object of integer fields, e.g. {"rounds": 3, "editSeconds": 90}.
// Missing fields keep the values in `base`. Malformed JSON, unknown keys and
// out-of-range values throw std::invalid_argument.
MatchSettings parseMatchSettings(const std::string& json, const MatchSettings& base = {});

// Reads and parses a settings file. Throws std::runtime_error if it can't be read.
MatchSettings loadMatchSettings(const std::string& path);

std::string toJson(const MatchSettings& settings);

} // namespace tta
