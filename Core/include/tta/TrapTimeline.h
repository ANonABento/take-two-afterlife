#pragma once

#include "tta/Placement.h"

#include <array>
#include <cstdint>

namespace tta {

// Deterministic trap timelines (plan req 3). Time is quantised to whole
// milliseconds, so pose(t) == pose(t + period) exactly and every machine
// computes the same pose for the same settings and time. No randomness.
struct TrapPose {
    double angleDeg = 0.0;          // hammer swing / spinner arm / trapdoor flap
    double extensionMeters = 0.0;   // spinner arm length, pusher reach, banana slide
    double openFraction = 0.0;      // trapdoor: 0 closed, 1 open
    bool projectileActive = false;  // cannon ball in flight
    double projectileDistanceMeters = 0.0;
    double projectileRadiusMeters = 0.0;
    int count = 1;                  // banana peels
    bool operator==(const TrapPose& o) const;
};

// Loop length in ms; 0 means the trap is static (banana).
std::int64_t trapPeriodMs(TrapType type, const std::array<int, 2>& settings);
// Throws std::invalid_argument when a setting is outside the catalogue range.
TrapPose pose(TrapType type, const std::array<int, 2>& settings, double tSeconds);

} // namespace tta
