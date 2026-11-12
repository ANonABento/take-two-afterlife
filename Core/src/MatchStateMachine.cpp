#include "tta/MatchStateMachine.h"

namespace tta {

const char* phaseName(Phase phase) {
    switch (phase) {
        case Phase::Lobby: return "Lobby";
        case Phase::Build: return "Build";
        case Phase::Run: return "Run";
        case Phase::Edit: return "Edit";
        case Phase::Premiere: return "Premiere";
        case Phase::Podium: return "Podium";
    }
    return "Unknown";
}

MatchStateMachine::MatchStateMachine(MatchSettings settings) : settings_(settings) {}

bool MatchStateMachine::canTransition(Phase to) const {
    const bool lastRound = round_ >= settings_.rounds;
    switch (phase_) {
        case Phase::Lobby: return to == Phase::Build;
        case Phase::Build: return to == Phase::Run;
        case Phase::Run: return to == Phase::Edit;
        case Phase::Edit: return to == Phase::Premiere;
        case Phase::Premiere: return lastRound ? to == Phase::Podium : to == Phase::Build;
        case Phase::Podium: return to == Phase::Lobby;
    }
    return false;
}

bool MatchStateMachine::transition(Phase to) {
    if (!canTransition(to)) return false;
    if (to == Phase::Build) ++round_;
    if (to == Phase::Lobby) round_ = 0;
    phase_ = to;
    return true;
}

int MatchStateMachine::phaseSeconds() const {
    switch (phase_) {
        case Phase::Build: return settings_.buildSeconds;
        case Phase::Run: return settings_.runSeconds;
        case Phase::Edit: return settings_.editSeconds;
        default: return 0;
    }
}

} // namespace tta
