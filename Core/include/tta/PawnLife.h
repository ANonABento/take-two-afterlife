#pragma once

namespace tta {

// Runner life cycle on a hit (plan req 8): impact frame, ragdoll, respawn at start.
constexpr double kImpactFreezeSeconds = 0.12;
constexpr double kRagdollSeconds = 1.5;

enum class LifeState { Alive, Frozen, Ragdoll };
enum class LifeEvent { None, StartRagdoll, Respawn };
enum class Form { Angel, Human };

// Angels wear wings and halo; human (runner) form drops both.
bool wingsVisible(Form form);
bool haloVisible(Form form);

class PawnLife {
public:
    // Alive -> Frozen. Returns false (and changes nothing) when not Alive.
    bool hit();
    // Advances timers by dt seconds (dt >= 0, else std::invalid_argument).
    // At most one transition per call; leftover time in that call is dropped.
    LifeEvent advance(double dt);
    LifeState state() const { return state_; }

private:
    LifeState state_ = LifeState::Alive;
    double elapsed_ = 0.0;
};

}  // namespace tta
