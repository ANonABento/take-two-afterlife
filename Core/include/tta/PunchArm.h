#pragma once

#include "tta/Punch.h"

namespace tta {

// Local bone rotation in degrees (UE FRotator order: pitch, yaw, roll).
struct ArmRotation {
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;
};

// Right-arm punch pose (punch arm task). Rest is a zero rotation on both bones.
struct PunchArmConfig {
    const char* upperArmBone = "UpperArm_R";
    const char* forearmBone = "ForeArm_R";
    ArmRotation windUpUpperArm{-55.0, 25.0, 0.0};   // end of WindUp
    ArmRotation windUpForearm{0.0, -120.0, 0.0};
    ArmRotation activeUpperArm{-80.0, -10.0, 0.0};  // end of Active, full extension
    ArmRotation activeForearm{};                    // straight
};

struct PunchArmPose {
    ArmRotation upperArm;
    ArmRotation forearm;
    double alpha = 0.0;  // 1 while punching, 0 when Idle
};

// Length of a punch phase in seconds (0 for Idle).
double punchPhaseSeconds(PunchPhase phase);

// Pose at normalised time t (clamped to 0..1) through phase. WindUp and
// Recovery ease in and out; Active starts fast and slows into extension.
PunchArmPose punchArmPose(PunchPhase phase, double t, const PunchArmConfig& config = PunchArmConfig{});

}  // namespace tta
