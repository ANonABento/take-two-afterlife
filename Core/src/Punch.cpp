#include "tta/Punch.h"

#include <stdexcept>

namespace tta {

namespace {
// Summed float steps land a hair short of exact phase lengths.
constexpr double kEpsilon = 1e-9;
}  // namespace

bool Punch::start(LifeState puncher) {
    if (phase_ != PunchPhase::Idle || cooldown_ > 0.0 || puncher != LifeState::Alive) return false;
    phase_ = PunchPhase::WindUp;
    elapsed_ = 0.0;
    cooldown_ = kPunchCooldownSeconds;
    return true;
}

PunchEvent Punch::advance(double dt) {
    if (dt < 0.0) throw std::invalid_argument("Punch::advance: negative dt");
    cooldown_ = cooldown_ - dt > kEpsilon ? cooldown_ - dt : 0.0;
    if (phase_ == PunchPhase::Idle) return PunchEvent::None;
    elapsed_ += dt;
    if (phase_ == PunchPhase::WindUp && elapsed_ + kEpsilon >= kPunchWindUpSeconds) {
        phase_ = PunchPhase::Active;
        elapsed_ = 0.0;
        return PunchEvent::OpenHitWindow;
    }
    if (phase_ == PunchPhase::Active && elapsed_ + kEpsilon >= kPunchActiveSeconds) {
        phase_ = PunchPhase::Recovery;
        elapsed_ = 0.0;
        return PunchEvent::CloseHitWindow;
    }
    if (phase_ == PunchPhase::Recovery && elapsed_ + kEpsilon >= kPunchRecoverySeconds) {
        phase_ = PunchPhase::Idle;
        elapsed_ = 0.0;
        return PunchEvent::Done;
    }
    return PunchEvent::None;
}

}  // namespace tta
