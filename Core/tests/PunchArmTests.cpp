#include "doctest/doctest.h"
#include "tta/PunchArm.h"

#include <string>

using namespace tta;

namespace {
void checkRot(const ArmRotation& r, double pitch, double yaw, double roll) {
    CHECK(r.pitch == doctest::Approx(pitch));
    CHECK(r.yaw == doctest::Approx(yaw));
    CHECK(r.roll == doctest::Approx(roll));
}
}  // namespace

TEST_CASE("punch arm config names the right-arm bones") {
    const PunchArmConfig c;
    CHECK(std::string(c.upperArmBone) == "UpperArm_R");
    CHECK(std::string(c.forearmBone) == "ForeArm_R");
}

TEST_CASE("wind-up pose at start, middle, end") {
    PunchArmPose p = punchArmPose(PunchPhase::WindUp, 0.0);
    checkRot(p.upperArm, 0, 0, 0);
    checkRot(p.forearm, 0, 0, 0);
    CHECK(p.alpha == 1.0);
    p = punchArmPose(PunchPhase::WindUp, 0.5);
    checkRot(p.upperArm, -27.5, 12.5, 0);
    checkRot(p.forearm, 0, -60, 0);
    p = punchArmPose(PunchPhase::WindUp, 1.0);
    checkRot(p.upperArm, -55, 25, 0);
    checkRot(p.forearm, 0, -120, 0);
}

TEST_CASE("active pose at start, middle, end") {
    PunchArmPose p = punchArmPose(PunchPhase::Active, 0.0);
    checkRot(p.upperArm, -55, 25, 0);
    checkRot(p.forearm, 0, -120, 0);
    p = punchArmPose(PunchPhase::Active, 0.5);  // easeOut(0.5) = 0.875
    checkRot(p.upperArm, -76.875, -5.625, 0);
    checkRot(p.forearm, 0, -15, 0);
    p = punchArmPose(PunchPhase::Active, 1.0);
    checkRot(p.upperArm, -80, -10, 0);
    checkRot(p.forearm, 0, 0, 0);
    CHECK(p.alpha == 1.0);
}

TEST_CASE("recovery pose at start, middle, end") {
    PunchArmPose p = punchArmPose(PunchPhase::Recovery, 0.0);
    checkRot(p.upperArm, -80, -10, 0);
    checkRot(p.forearm, 0, 0, 0);
    p = punchArmPose(PunchPhase::Recovery, 0.5);
    checkRot(p.upperArm, -40, -5, 0);
    p = punchArmPose(PunchPhase::Recovery, 1.0);
    checkRot(p.upperArm, 0, 0, 0);
    checkRot(p.forearm, 0, 0, 0);
}

TEST_CASE("easing: wind-up and recovery start slow, punch starts fast") {
    // 10% of the time covers less than 10% of the wind-up angle...
    CHECK(punchArmPose(PunchPhase::WindUp, 0.1).upperArm.pitch > -5.5);
    CHECK(punchArmPose(PunchPhase::Recovery, 0.9).upperArm.pitch > -8.0);
    // ...but the first quarter of the punch covers more than a quarter of it.
    CHECK(punchArmPose(PunchPhase::Active, 0.25).forearm.yaw > -90.0);
}

TEST_CASE("idle is rest with zero alpha; t is clamped") {
    const PunchArmPose idle = punchArmPose(PunchPhase::Idle, 0.5);
    checkRot(idle.upperArm, 0, 0, 0);
    CHECK(idle.alpha == 0.0);
    checkRot(punchArmPose(PunchPhase::WindUp, 2.0).upperArm, -55, 25, 0);
    checkRot(punchArmPose(PunchPhase::WindUp, -1.0).upperArm, 0, 0, 0);
}

TEST_CASE("phase lengths come from the punch timings") {
    CHECK(punchPhaseSeconds(PunchPhase::WindUp) == kPunchWindUpSeconds);
    CHECK(punchPhaseSeconds(PunchPhase::Active) == kPunchActiveSeconds);
    CHECK(punchPhaseSeconds(PunchPhase::Recovery) == kPunchRecoverySeconds);
    CHECK(punchPhaseSeconds(PunchPhase::Idle) == 0.0);
}
