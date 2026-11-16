#pragma once

namespace tta {

// Runner life cycle on a hit (plan req 8): impact frame, ragdoll, respawn at start.
constexpr double kImpactFreezeSeconds = 0.12;
constexpr double kRagdollSeconds = 2.5;   // long enough to watch the body land before the respawn
// Punch stun (punch plan req 3-4): short ragdoll, get up in place, then immunity.
constexpr double kStunSeconds = 1.0;
constexpr double kPunchImmunitySeconds = 2.0;

enum class LifeState { Alive, Frozen, Ragdoll, Stunned };
// GetUp = stun over, stand up in place (no respawn, no score change).
enum class LifeEvent { None, StartRagdoll, Respawn, GetUp };
enum class Form { Angel, Human };

// Angels wear wings and halo; human (runner) form drops both.
bool wingsVisible(Form form);
bool haloVisible(Form form);

class PawnLife {
public:
    // Alive or Stunned -> Frozen (a trap beats a punch stun). Returns false
    // (and changes nothing) otherwise.
    bool hit();
    // Alive -> Stunned for `seconds` (> 0, else std::invalid_argument).
    // Returns false (and changes nothing) when not Alive or punch-immune.
    bool stun(double seconds);
    // True for kPunchImmunitySeconds after getting up from a stun.
    bool punchImmune() const { return immunity_ > 0.0; }
    // Advances timers by dt seconds (dt >= 0, else std::invalid_argument).
    // At most one transition per call; leftover time in that call is dropped.
    LifeEvent advance(double dt);
    LifeState state() const { return state_; }

private:
    LifeState state_ = LifeState::Alive;
    double elapsed_ = 0.0;
    double stunSeconds_ = 0.0;
    double immunity_ = 0.0;
};

}  // namespace tta
