#include "tta/PunchArm.h"

#include <algorithm>

namespace tta {

namespace {

ArmRotation lerp(const ArmRotation& a, const ArmRotation& b, double k) {
    return {a.pitch + (b.pitch - a.pitch) * k, a.yaw + (b.yaw - a.yaw) * k, a.roll + (b.roll - a.roll) * k};
}

double easeInOut(double t) { return t * t * (3.0 - 2.0 * t); }

double easeOut(double t) {
    const double u = 1.0 - t;
    return 1.0 - u * u * u;
}

}  // namespace

double punchPhaseSeconds(PunchPhase phase) {
    switch (phase) {
        case PunchPhase::WindUp: return kPunchWindUpSeconds;
        case PunchPhase::Active: return kPunchActiveSeconds;
        case PunchPhase::Recovery: return kPunchRecoverySeconds;
        case PunchPhase::Idle: break;
    }
    return 0.0;
}

PunchArmPose punchArmPose(PunchPhase phase, double t, const PunchArmConfig& config) {
    const double k = std::clamp(t, 0.0, 1.0);
    const ArmRotation rest{};
    PunchArmPose pose;
    switch (phase) {
        case PunchPhase::WindUp:
            pose.upperArm = lerp(rest, config.windUpUpperArm, easeInOut(k));
            pose.forearm = lerp(rest, config.windUpForearm, easeInOut(k));
            pose.alpha = 1.0;
            break;
        case PunchPhase::Active:
            pose.upperArm = lerp(config.windUpUpperArm, config.activeUpperArm, easeOut(k));
            pose.forearm = lerp(config.windUpForearm, config.activeForearm, easeOut(k));
            pose.alpha = 1.0;
            break;
        case PunchPhase::Recovery:
            pose.upperArm = lerp(config.activeUpperArm, rest, easeInOut(k));
            pose.forearm = lerp(config.activeForearm, rest, easeInOut(k));
            pose.alpha = 1.0;
            break;
        case PunchPhase::Idle: break;
    }
    return pose;
}

}  // namespace tta
