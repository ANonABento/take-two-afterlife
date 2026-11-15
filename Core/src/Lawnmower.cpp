#include "tta/Lawnmower.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tta {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

Lawnmower::Lawnmower(const Placement& trap) : trap_(trap) {
    if (trap.isCamera || trap.trap != TrapType::Lawnmower) throw std::invalid_argument("Lawnmower needs a lawnmower placement");
}

double Lawnmower::driveOffsetMeters(std::int64_t ms) const {
    const double speed = trap_.settings[0], travel = trap_.settings[1];
    return std::clamp(speed * static_cast<double>(ms) / 1000.0, 0.0, travel);
}

void Lawnmower::toLocal(const RunnerSpot& r, double& x, double& y) const {
    const double a = trap_.rotation * kPi / 4.0;
    const double dx = r.xMeters - cellToMeters(trap_.cellX), dy = r.yMeters - cellToMeters(trap_.cellY);
    x = dx * std::cos(a) + dy * std::sin(a);
    y = -dx * std::sin(a) + dy * std::cos(a);
}

bool Lawnmower::inTriggerZone(const RunnerSpot& runner) const {
    double x = 0.0, y = 0.0;
    toLocal(runner, x, y);
    const double front = kLawnmowerLengthMeters / 2.0;
    return x >= front && x <= front + kLawnmowerZoneDepthMeters && std::fabs(y) <= kLawnmowerZoneWidthMeters / 2.0;
}

void Lawnmower::update(double tSeconds, const std::vector<RunnerSpot>& runners, RunRecorder& recorder) {
    if (!std::isfinite(tSeconds)) throw std::invalid_argument("lawnmower time must be finite");
    const std::int64_t ms = std::llround(tSeconds * 1000.0);
    if (ms < lastMs_) throw std::invalid_argument("lawnmower time went backwards");
    lastMs_ = ms;
    if (state_ == LawnmowerState::Spent) return;
    if (state_ == LawnmowerState::Idle) {
        if (std::none_of(runners.begin(), runners.end(), [this](const RunnerSpot& r) { return inTriggerZone(r); })) return;
        state_ = LawnmowerState::Driving;
        triggerMs_ = ms;
    }
    offset_ = driveOffsetMeters(ms - triggerMs_);
    for (const RunnerSpot& r : runners) {
        double x = 0.0, y = 0.0;
        toLocal(r, x, y);
        const bool overlaps = std::fabs(x - offset_) <= kLawnmowerLengthMeters / 2.0 && std::fabs(y) <= kLawnmowerWidthMeters / 2.0;
        if (overlaps && hit_.insert(r.id).second) recorder.recordHit(trap_, r.id);
    }
    if (offset_ >= trap_.settings[1]) state_ = LawnmowerState::Spent;
}

} // namespace tta
