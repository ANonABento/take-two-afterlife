#include "tta/Scoring.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace tta {

namespace {
void requirePlayer(const RoundInput& round, int player, const char* role) {
    if (round.players.count(player) == 0)
        throw std::invalid_argument(std::string(role) + " " + std::to_string(player) + " is not in the round");
}
} // namespace

bool trapPointsVoided(const RoundInput& round) {
    return round.finishers.empty() || round.finishers.size() == round.players.size();
}

std::map<int, double> scoreRound(const RoundInput& round) {
    std::map<int, double> points;
    for (int p : round.players) points[p] = 0.0;
    for (int p : round.finishers) {
        requirePlayer(round, p, "finisher");
        points[p] += kGatePoints;
    }
    const bool voided = trapPointsVoided(round);
    for (const Wipeout& w : round.wipeouts) {
        requirePlayer(round, w.victim, "victim");
        requirePlayer(round, w.trapOwner, "trap owner");
        if (voided || w.victim == w.trapOwner) continue;
        points[w.trapOwner] += std::max(kMinTrapPoints, static_cast<double>(trapSpec(w.trap).cost));
    }
    for (const auto& [p, r] : round.ratingPoints) {
        requirePlayer(round, p, "rated player");
        points[p] += r;
    }
    return points;
}

std::map<int, double> MatchScore::addRound(const RoundInput& round) {
    std::map<int, double> points = scoreRound(round);
    for (const auto& [p, v] : points) totals_[p] += v;
    ++rounds_;
    return points;
}

double MatchScore::total(int player) const {
    auto it = totals_.find(player);
    return it == totals_.end() ? 0.0 : it->second;
}

std::vector<int> MatchScore::podium() const {
    std::vector<int> order;
    for (const auto& [p, v] : totals_) order.push_back(p);
    std::stable_sort(order.begin(), order.end(),
                     [this](int a, int b) { return totals_.at(a) > totals_.at(b); });
    return order;
}

} // namespace tta
