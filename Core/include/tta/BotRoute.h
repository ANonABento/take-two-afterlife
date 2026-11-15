#pragma once

#include <cstddef>
#include <vector>

namespace tta {

// Engine-free steering for a bot runner (MVP req 3): walk marker to marker toward the gate,
// jump at gaps and step-ups, and ask for a respawn when stuck. Metres, world axes.
struct RoutePoint {
    double x = 0, y = 0, z = 0;
};

struct RouteInput {
    RoutePoint pos;
    bool groundAhead = true; // the engine's floor probe a short way ahead
    bool skipJump = false;   // a deliberate fail: don't jump this gap
    double dt = 0;           // seconds since the last step, >= 0
};

struct RouteStep {
    double dirX = 0, dirY = 0; // unit move direction on the ground plane; 0,0 when done
    bool jump = false;
    bool respawn = false; // no progress for kStuckSeconds
    bool done = false;    // every point reached
};

class BotRoute {
public:
    static constexpr double kReachMeters = 1.0;
    static constexpr double kStepUpMeters = 0.4;
    static constexpr double kStuckSeconds = 3.0;
    static constexpr double kProgressMeters = 0.5;

    // Throws std::invalid_argument when `points` is empty.
    explicit BotRoute(std::vector<RoutePoint> points);

    // Throws std::invalid_argument when in.dt < 0.
    RouteStep step(const RouteInput& in);
    // Back to the first point (after a respawn at the start).
    void restart();
    std::size_t target() const { return target_; }
    std::size_t size() const { return points_.size(); }

private:
    std::vector<RoutePoint> points_;
    std::size_t target_ = 0;
    double best_ = -1; // closest distance to the current target since the last progress
    double stalled_ = 0;
};

} // namespace tta
