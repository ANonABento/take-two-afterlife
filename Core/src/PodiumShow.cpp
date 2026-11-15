#include "tta/PodiumShow.h"

#include <stdexcept>

namespace tta {

PodiumShow::PodiumShow(const std::vector<int>& ranking, bool inHeaven, double holdSeconds)
    : statues_(ranking.begin(), ranking.begin() + static_cast<std::ptrdiff_t>(std::min(ranking.size(), kStatues))),
      holdSeconds_(holdSeconds),
      step_(inHeaven ? PodiumStep::Statues : PodiumStep::Travel) {
    if (holdSeconds < 0) throw std::invalid_argument("podium hold can't be negative");
}

void PodiumShow::arrived() {
    if (step_ != PodiumStep::Travel) throw std::logic_error("podium: arrived outside Travel");
    step_ = PodiumStep::Statues;
}

void PodiumShow::statuesPlaced() {
    if (step_ != PodiumStep::Statues) throw std::logic_error("podium: statues placed outside Statues");
    step_ = PodiumStep::Hold;
}

void PodiumShow::tick(double seconds) {
    if (step_ != PodiumStep::Hold) return;
    held_ += seconds;
    if (held_ >= holdSeconds_) step_ = PodiumStep::Lobby;
}

bool statuesStand(Phase phase) { return phase == Phase::Podium || phase == Phase::Lobby; }

} // namespace tta
