#include "tta/BotRoute.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace tta {

BotRoute::BotRoute(std::vector<RoutePoint> points) : points_(std::move(points)) {
    if (points_.empty()) throw std::invalid_argument("BotRoute needs at least one point");
}

void BotRoute::restart() {
    target_ = 0;
    best_ = -1;
    stalled_ = 0;
}

RouteStep BotRoute::step(const RouteInput& in) {
    if (!(in.dt >= 0)) throw std::invalid_argument("BotRoute::step dt must be >= 0");
    RouteStep out;
    auto flat = [&](const RoutePoint& p) { return std::hypot(p.x - in.pos.x, p.y - in.pos.y); };
    // Past a marker along the route counts as reaching it, so a bot that runs by
    // without touching it keeps going instead of turning back.
    auto passed = [&](std::size_t i) {
        if (i == 0) return false;
        const RoutePoint& a = points_[i - 1];
        const RoutePoint& b = points_[i];
        return (in.pos.x - b.x) * (b.x - a.x) + (in.pos.y - b.y) * (b.y - a.y) > 0;
    };
    while (target_ < points_.size() && (flat(points_[target_]) <= kReachMeters || passed(target_))) {
        ++target_;
        best_ = -1;
        stalled_ = 0;
    }
    if (target_ == points_.size()) {
        out.done = true;
        return out;
    }
    const RoutePoint& goal = points_[target_];
    const double dist = flat(goal);
    out.dirX = (goal.x - in.pos.x) / dist;
    out.dirY = (goal.y - in.pos.y) / dist;
    out.jump = (!in.groundAhead && !in.skipJump) || goal.z - in.pos.z > kStepUpMeters;
    if (best_ < 0 || dist < best_ - kProgressMeters) {
        best_ = dist;
        stalled_ = 0;
    } else if ((stalled_ += in.dt) >= kStuckSeconds) {
        out.respawn = true;
        stalled_ = 0;
    }
    return out;
}

} // namespace tta
