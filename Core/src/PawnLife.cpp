#include "tta/PawnLife.h"

#include <stdexcept>

namespace tta {

bool wingsVisible(Form form) { return form == Form::Angel; }
bool haloVisible(Form form) { return form == Form::Angel; }

bool PawnLife::hit() {
    if (state_ != LifeState::Alive) return false;
    state_ = LifeState::Frozen;
    elapsed_ = 0.0;
    return true;
}

LifeEvent PawnLife::advance(double dt) {
    if (dt < 0.0) throw std::invalid_argument("PawnLife::advance: negative dt");
    if (state_ == LifeState::Alive) return LifeEvent::None;
    elapsed_ += dt;
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
