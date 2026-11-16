#pragma once

#include "tta/BotBrain.h"

#include <cstdint>
#include <vector>

namespace tta {

struct RoutePoint {
    double x = 0, y = 0, z = 0;
};

// One race step for a bot that does not follow route markers. Metres, world axes.
// groundLeft / groundRight are floor probes on the runner's left and right.
// `rightX/rightY` is that runner's ground-plane right vector.
struct RaceSense {
    RoutePoint pos;
    RoutePoint gate;
    double rightX = 0;
    double rightY = 1;
    bool groundLeft = true;
    bool groundCenter = true;
    bool groundRight = true;
    bool refuseJump = false; // deliberate miss on a gap
    bool punchReady = false;
    RunnerSkill skill = RunnerSkill::Average;
    double dt = 0;
    std::vector<RoutePoint> hazards;
    std::vector<RoutePoint> rivals;
};

struct RaceAction {
    double dirX = 0, dirY = 0; // unit move on the ground plane; 0,0 when the gate is reached
    bool jump = false;
    bool punch = false;
    bool respawn = false; // no progress toward the gate for kGiveUpSeconds
    bool done = false;
};

// Seeded so a stall sidestep is the same on every machine. Build placement stays in BotBrain.
class BotRace {
public:
    static constexpr double kGateMeters = 0.4;
    static constexpr double kProgressMeters = 0.4;
    static constexpr double kSidestepSeconds = 1.5;
    static constexpr double kGiveUpSeconds = 4.0;

    explicit BotRace(std::uint64_t seed);

    // Throws std::invalid_argument when in.dt < 0.
    RaceAction step(const RaceSense& in);
    void restart();

private:
    std::uint64_t next();

    std::uint64_t state_;
    bool haveBest_ = false;
    double bestAlong_ = 0;
    double stalled_ = 0;
    bool sidestepping_ = false;
    int sidestep_ = 1;
};

} // namespace tta
