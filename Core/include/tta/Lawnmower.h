#pragma once

#include "tta/Placement.h"
#include "tta/RunRecorder.h"

#include <cstdint>
#include <set>
#include <vector>

namespace tta {

// One-shot lawnmower trap (trap swap plan req 6). Idle until a runner enters
// the zone in front of it, then drives along its local +X at `speed` m/s for
// `travel` m (settings 0 and 1), then stays Spent for the round. Its offset is a
// pure function of the whole-millisecond trigger time, so replays match.
constexpr double kLawnmowerZoneDepthMeters = 3.0;
constexpr double kLawnmowerZoneWidthMeters = 1.5;
constexpr double kLawnmowerLengthMeters = 1.0; // deck footprint, matches SM_Trap_Lawnmower
constexpr double kLawnmowerWidthMeters = 0.6;

enum class LawnmowerState { Idle, Driving, Spent };

struct RunnerSpot {
    int id = 0;
    double xMeters = 0.0;
    double yMeters = 0.0;
};

class Lawnmower {
public:
    // Throws std::invalid_argument unless `trap` is a lawnmower.
    explicit Lawnmower(const Placement& trap);

    // Steps to run time t (seconds, never backwards): triggers when Idle and a
    // runner is in the zone, moves while Driving, and records one hit per runner
    // it overlaps on `recorder` (whose clock the caller keeps at t).
    void update(double tSeconds, const std::vector<RunnerSpot>& runners, RunRecorder& recorder);

    LawnmowerState state() const { return state_; }
    double offsetMeters() const { return offset_; }
    // Distance driven `ms` after the trigger, clamped to the travel distance.
    double driveOffsetMeters(std::int64_t ms) const;
    bool inTriggerZone(const RunnerSpot& runner) const;

private:
    void toLocal(const RunnerSpot& r, double& x, double& y) const;
    Placement trap_;
    LawnmowerState state_ = LawnmowerState::Idle;
    std::int64_t triggerMs_ = 0;
    std::int64_t lastMs_ = 0;
    double offset_ = 0.0;
    std::set<int> hit_;
};

} // namespace tta
