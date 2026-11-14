#include "tta/MatchFlow.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace tta {

namespace {
int toPlayerId(int replayId, const std::vector<int>& ids) {
    if (replayId < 0 || static_cast<std::size_t>(replayId) >= ids.size())
        throw std::invalid_argument("replay id " + std::to_string(replayId) + " has no player");
    return ids[static_cast<std::size_t>(replayId)];
}
} // namespace

bool canJoin(Phase phase) {
    return phase == Phase::Lobby || phase == Phase::Premiere || phase == Phase::Podium;
}

std::vector<int> runPlayers(const std::vector<int>& runners, const Layout& layout) {
    std::vector<int> ids = runners;
    for (const Placement& p : layout.all()) {
        if (p.isCamera || std::find(ids.begin(), ids.end(), p.owner) != ids.end()) continue;
        ids.push_back(p.owner);
    }
    return ids;
}

RoundInput toPlayerIds(const RoundInput& run, const std::vector<int>& ids, std::size_t runnerCount) {
    RoundInput out;
    for (int r : run.players) {
        const int p = toPlayerId(r, ids);
        if (static_cast<std::size_t>(r) < runnerCount) out.players.insert(p);
        else out.absentOwners.insert(p);
    }
    for (int r : run.absentOwners) out.absentOwners.insert(toPlayerId(r, ids));
    for (int r : run.finishers) out.finishers.insert(toPlayerId(r, ids));
    for (const Wipeout& w : run.wipeouts)
        out.wipeouts.push_back({toPlayerId(w.victim, ids), toPlayerId(w.trapOwner, ids), w.trap});
    for (const auto& [r, v] : run.ratingPoints) out.ratingPoints[toPlayerId(r, ids)] += v;
    return out;
}

bool returnToLobby(MatchStateMachine& match, MatchScore& score, const std::vector<int>& players) {
    if (match.phase() != Phase::Podium || !match.transition(Phase::Lobby)) return false;
    score.reset(players);
    return true;
}

} // namespace tta
