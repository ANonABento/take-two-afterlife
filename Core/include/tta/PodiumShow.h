#pragma once

#include "tta/MatchStateMachine.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace tta {

enum class PodiumStep { Travel, Statues, Hold, Lobby };

// What happens after round 3's premiere, in order: travel to Heaven (skipped when
// already there), spawn the top 3 as frozen statues on the podium, hold on them,
// then everyone goes back to the Heaven lobby. The statues stay through the lobby.
class PodiumShow {
public:
    static constexpr std::size_t kStatues = 3;

    // `ranking` = MatchScore::podium(). holdSeconds < 0 throws std::invalid_argument.
    PodiumShow(const std::vector<int>& ranking, bool inHeaven, double holdSeconds);

    PodiumStep step() const { return step_; }
    // Top 3 (fewer with fewer players), winner first.
    const std::vector<int>& statues() const { return statues_; }
    // Travel -> Statues; std::logic_error from any other step.
    void arrived();
    // Statues -> Hold; std::logic_error from any other step.
    void statuesPlaced();
    // Hold -> Lobby once holdSeconds have passed; no-op elsewhere.
    void tick(double seconds);

private:
    std::vector<int> statues_;
    double holdSeconds_;
    double held_ = 0;
    PodiumStep step_;
};

// Podium statues stand in Podium and Lobby and go when the next match starts (Build).
bool statuesStand(Phase phase);

} // namespace tta
