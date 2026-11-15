#pragma once

#include "tta/PawnLife.h"

namespace tta {

// Click-to-punch timing (punch plan req 1). Cooldown runs from the press.
constexpr double kPunchWindUpSeconds = 0.45;
constexpr double kPunchActiveSeconds = 0.10;
constexpr double kPunchRecoverySeconds = 0.35;
constexpr double kPunchCooldownSeconds = 1.5;

enum class PunchPhase { Idle, WindUp, Active, Recovery };
// OpenHitWindow/CloseHitWindow bracket the Active phase; Done = back to Idle.
enum class PunchEvent { None, OpenHitWindow, CloseHitWindow, Done };

class Punch {
public:
    // Idle -> WindUp. Returns false (and changes nothing) unless Idle, off
    // cooldown and the puncher is Alive.
    bool start(LifeState puncher);
    // Advances timers by dt seconds (dt >= 0, else std::invalid_argument).
    // At most one phase transition per call; leftover phase time is dropped.
    PunchEvent advance(double dt);
    PunchPhase phase() const { return phase_; }
    bool hitWindowOpen() const { return phase_ == PunchPhase::Active; }
    double cooldownLeft() const { return cooldown_; }

private:
    PunchPhase phase_ = PunchPhase::Idle;
    double elapsed_ = 0.0;
    double cooldown_ = 0.0;
};

}  // namespace tta
