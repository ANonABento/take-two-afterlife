#include "tta/RunRecorder.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tta {

namespace {
std::uint32_t runLengthMs(double runSeconds) {
    if (!(runSeconds > 0.0)) throw std::invalid_argument("run length must be positive");
    return static_cast<std::uint32_t>(std::llround(runSeconds * 1000.0));
}
} // namespace

RunRecorder::RunRecorder(std::vector<int> players, double runSeconds)
    : replay_(std::move(players)), runMs_(runLengthMs(runSeconds)) {}

int RunRecorder::advance(double deltaSeconds) {
    if (deltaSeconds < 0.0) throw std::invalid_argument("negative frame delta");
    elapsedSeconds_ += deltaSeconds;
    elapsedMs_ = static_cast<std::uint32_t>(std::floor(elapsedSeconds_ * 1000.0 + 1e-6));
    const int target = static_cast<int>(elapsedMs_ / kReplayFrameMs) + 1; // frame 0 at t = 0
    const int due = target - framesDue_;
    framesDue_ = target;
    return due;
}

void RunRecorder::recordHit(const Placement& trap, int victim) {
    if (trap.isCamera) throw std::invalid_argument("a camera can't hit a runner");
    replay_.addEvent({ReplayEventType::Hit, elapsedMs_, victim, trap.id, trap.owner});
}

std::vector<int> RunRecorder::takeSmites(const std::vector<int>& unfinished) {
    if (!timedOut() || smitten_) return {};
    smitten_ = true;
    for (const int victim : unfinished) replay_.addEvent({ReplayEventType::Smite, runMs_, victim, kNoTrap, kNoTrap});
    return unfinished;
}

} // namespace tta
