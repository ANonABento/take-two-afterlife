#include "tta/SpawnPlacement.h"

namespace tta {

std::vector<double> spawnLifts() {
    std::vector<double> lifts;
    for (double extra = 0.0; extra <= kSpawnExtraLiftCm; extra += kSpawnLiftStepCm) {
        lifts.push_back(kSpawnLiftCm + extra);
    }
    return lifts;
}

SpawnPick pickSpawn(const std::vector<SpawnFloor>& starts, const std::function<bool(int, double)>& blocked) {
    const std::vector<double> lifts = spawnLifts();
    for (int i = 0; i < static_cast<int>(starts.size()); ++i) {
        const SpawnFloor& s = starts[static_cast<size_t>(i)];
        const double base = s.hit ? s.floorZ : s.startZ;
        for (double lift : lifts) {
            const double z = base + lift;
            if (!blocked || !blocked(i, z)) return SpawnPick{true, i, z};
        }
    }
    return SpawnPick{};
}

}  // namespace tta
