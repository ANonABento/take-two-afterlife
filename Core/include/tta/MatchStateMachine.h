#pragma once

#include "tta/MatchSettings.h"

namespace tta {

enum class Phase { Lobby, Build, Run, Edit, Premiere, Podium };

const char* phaseName(Phase phase);

// Lobby -> Build -> Run -> Edit -> Premiere -> (Build of next round | Podium) -> Lobby.
// Illegal transitions are rejected and leave the state unchanged.
class MatchStateMachine {
public:
    explicit MatchStateMachine(MatchSettings settings);

    Phase phase() const { return phase_; }
    // 1-based round number; 0 while in Lobby.
    int round() const { return round_; }
    const MatchSettings& settings() const { return settings_; }

    bool canTransition(Phase to) const;
    // Returns false (and changes nothing) for an illegal transition.
    bool transition(Phase to);

    // Timer for the current phase in seconds; 0 for untimed phases (Lobby, Premiere, Podium).
    int phaseSeconds() const;

private:
    MatchSettings settings_;
    Phase phase_ = Phase::Lobby;
    int round_ = 0;
};

} // namespace tta
