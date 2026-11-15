#include "tta/BotBrain.h"

namespace tta {

namespace {
constexpr int kBuildAttempts = 64;
constexpr int kBotCameras = 3;
constexpr int kCorridorCellsX = static_cast<int>(kCorridorLengthMeters / kGridMeters);
constexpr int kHalfWidthCells = static_cast<int>(kCorridorWidthMeters / 2 / kGridMeters);
constexpr int kCameraTypes = 4; // CameraType::Fixed..TrapCam
} // namespace

BotBrain::BotBrain(int playerId, std::uint64_t seed)
    : playerId_(playerId), state_(seed ^ (0x9E3779B97F4A7C15ULL * static_cast<std::uint64_t>(playerId + 1))) {}

std::uint64_t BotBrain::next() {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int BotBrain::range(int lo, int hi) {
    return lo + static_cast<int>(next() % static_cast<std::uint64_t>(hi - lo + 1));
}

double BotBrain::unit() { return static_cast<double>(next() >> 11) / 9007199254740992.0; }

void BotBrain::build(Layout& layout) {
    const auto& traps = trapCatalogue();
    for (int i = 0; i < kBuildAttempts; ++i) {
        const TrapSpec& spec = traps[static_cast<size_t>(range(0, static_cast<int>(traps.size()) - 1))];
        if (spec.cost > layout.remaining(playerId_)) continue; // try a cheaper one next
        layout.placeTrap(playerId_, spec.type, cellToMeters(range(0, kCorridorCellsX)),
                         cellToMeters(range(-kHalfWidthCells, kHalfWidthCells)), range(0, kRotationSteps - 1) * 45.0);
    }
    for (int i = 0; i < kBotCameras && layout.cameraCount(playerId_) < kCameraCap; ++i) {
        const auto type = static_cast<CameraType>(range(0, kCameraTypes - 1));
        layout.placeCamera(playerId_, type, cellToMeters(range(0, kCorridorCellsX)),
                           cellToMeters(range(-kHalfWidthCells, kHalfWidthCells)), range(0, kRotationSteps - 1) * 45.0);
    }
}

RunnerPlan BotBrain::planRun() {
    RunnerPlan plan;
    plan.skill = static_cast<RunnerSkill>(range(0, 2));
    plan.failChance = 0.05 + 0.25 * unit(); // 5-30%: bots sometimes fail on purpose for funny footage
    return plan;
}

void BotBrain::rate(RatingBook& book, const std::vector<std::pair<int, Cut>>& cuts) {
    for (const auto& [cutId, cut] : cuts) {
        if (const auto scores = vote(cut)) book.rate(cutId, playerId_, scores->first, scores->second);
    }
}

std::optional<std::pair<int, int>> BotBrain::vote(const Cut& cut) {
    if (cut.editor == playerId_ || cut.builder == playerId_) return std::nullopt;
    const int funny = range(kMinRating, kMaxRating);
    return std::make_pair(funny, range(kMinRating, kMaxRating));
}

bool BotBrain::rollFail(const RunnerPlan& plan) { return unit() < plan.failChance; }

} // namespace tta
