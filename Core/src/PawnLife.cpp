#include "tta/PawnLife.h"

#include <stdexcept>

namespace tta {

bool wingsVisible(Form form) { return form == Form::Angel; }
bool haloVisible(Form form) { return form == Form::Angel; }

bool PawnLife::hit() {
    if (state_ != LifeState::Alive && state_ != LifeState::Stunned) return false;
    state_ = LifeState::Frozen;
    elapsed_ = 0.0;
    immunity_ = 0.0;
    return true;
}

bool PawnLife::stun(double seconds) {
    if (!(seconds > 0.0)) throw std::invalid_argument("PawnLife::stun: seconds must be > 0");
    if (state_ != LifeState::Alive || punchImmune()) return false;
    state_ = LifeState::Stunned;
    elapsed_ = 0.0;
    stunSeconds_ = seconds;
    return true;
}

LifeEvent PawnLife::advance(double dt) {
    if (dt < 0.0) throw std::invalid_argument("PawnLife::advance: negative dt");
    if (state_ == LifeState::Alive) {
        // Same float-edge slack as Punch so 1.99 + 0.01 ends immunity.
        immunity_ = immunity_ - dt > 1e-9 ? immunity_ - dt : 0.0;
        return LifeEvent::None;
    }
    elapsed_ += dt;
    if (state_ == LifeState::Stunned && elapsed_ >= stunSeconds_) {
        state_ = LifeState::Alive;
        elapsed_ = 0.0;
        immunity_ = kPunchImmunitySeconds;
        return LifeEvent::GetUp;
    }
    if (state_ == LifeState::Frozen && elapsed_ >= kImpactFreezeSeconds) {
        state_ = LifeState::Ragdoll;
        elapsed_ = 0.0;
        return LifeEvent::StartRagdoll;
    }
    if (state_ == LifeState::Ragdoll && elapsed_ >= kRagdollSeconds) {
        state_ = LifeState::Alive;
        elapsed_ = 0.0;
        return LifeEvent::Respawn;
    }
    return LifeEvent::None;
}

}  // namespace tta
