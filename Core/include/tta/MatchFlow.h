#pragma once

#include "tta/MatchStateMachine.h"
#include "tta/Placement.h"
#include "tta/Scoring.h"

#include <cstddef>
#include <vector>

namespace tta {

// Match flow glue (plan req 7): who may join, and who counts in a Run.

// Players join only between rounds: Lobby, Premiere or Podium.
bool canJoin(Phase phase);

// Replay ids for a Run, as player ids by index: the runners in order, then every
// owner of a placed trap who isn't running (left the match), once each, in
// placement order. Their traps stay live and score for them.
std::vector<int> runPlayers(const std::vector<int>& runners, const Layout& layout);

// Maps a Run's RoundInput from replay ids (indices into `ids`) to player ids.
// Replay ids below `runnerCount` are runners (`players`); the rest become
// `absentOwners`. Throws std::invalid_argument on a replay id outside `ids`.
// Podium -> Lobby (plan req 4): same room, new match. Round goes back to 0 and scores
// to 0 for every player still in the room (`players`). False (nothing changes) outside Podium.
bool returnToLobby(MatchStateMachine& match, MatchScore& score, const std::vector<int>& players);

RoundInput toPlayerIds(const RoundInput& run, const std::vector<int>& ids, std::size_t runnerCount);

} // namespace tta
