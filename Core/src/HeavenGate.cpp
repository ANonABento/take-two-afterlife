#include "tta/HeavenGate.h"

#include <algorithm>

namespace tta {

void HeavenGate::setPlayers(int players) { players_ = std::max(0, players); }

bool HeavenGate::markReady(int playerId) {
    if (!open_ || !ready_.insert(playerId).second) return false;
    if (sinceFirstReady_ < 0.0) sinceFirstReady_ = 0.0;
    return true;
}

void HeavenGate::tick(double seconds) {
    if (sinceFirstReady_ >= 0.0) sinceFirstReady_ += seconds;
}

bool HeavenGate::shouldTravel() const {
    if (!open_ || ready_.empty()) return false;
    return readyCount() >= players_ || sinceFirstReady_ >= kTimeoutSeconds;
}

}  // namespace tta
