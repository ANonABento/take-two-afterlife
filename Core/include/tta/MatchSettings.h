#pragma once

#include <string>

namespace tta {

// Host-adjustable match settings, saved as JSON on the host's machine.
struct MatchSettings {
    int rounds = 3;
    int buildSeconds = 120;
    int runSeconds = 120;
    int editSeconds = 90;
    int ratingSeconds = 10;
};

// Parses a flat JSON object of integer fields, e.g. {"rounds": 3, "editSeconds": 90}.
// Missing fields keep their defaults. Malformed JSON, unknown keys and
// out-of-range values throw std::invalid_argument.
MatchSettings parseMatchSettings(const std::string& json);

// Reads and parses a settings file. Throws std::runtime_error if it can't be read.
MatchSettings loadMatchSettings(const std::string& path);

std::string toJson(const MatchSettings& settings);

} // namespace tta
