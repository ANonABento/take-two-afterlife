#pragma once

#include <functional>
#include <vector>

namespace tta {

// Lobby/course spawn placement. The pawn goes kSpawnLiftCm above the floor found under a
// PlayerStart and drops onto it; if the capsule is blocked there, it tries higher by
// kSpawnLiftStepCm up to kSpawnExtraLiftCm more, then moves on to the next PlayerStart.
constexpr double kSpawnLiftCm = 300.0;
constexpr double kSpawnExtraLiftCm = 400.0;
constexpr double kSpawnLiftStepCm = 100.0;

struct SpawnFloor {
    bool hit = false;     // the down trace found a floor
    double floorZ = 0.0;  // where it hit
    double startZ = 0.0;  // the PlayerStart's own Z, used when nothing was hit
};

struct SpawnPick {
    bool found = false;
    int start = -1;  // index into the starts passed to pickSpawn
    double z = 0.0;  // capsule centre Z
};

// Lift above the floor for each try, in order: 300, 400, ... 700.
std::vector<double> spawnLifts();

// starts[0] is the PlayerStart the game mode chose; the rest are fallbacks in order.
// blocked(startIndex, z) says whether the capsule centred at that Z overlaps something.
SpawnPick pickSpawn(const std::vector<SpawnFloor>& starts, const std::function<bool(int, double)>& blocked);

}  // namespace tta
