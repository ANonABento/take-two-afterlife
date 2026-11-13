#pragma once

#include "tta/Placement.h"

#include <map>
#include <set>
#include <vector>

namespace tta {

// Scoring rules (plan req 4).
constexpr double kGatePoints = 1.0;
constexpr double kMinTrapPoints = 1.0;

struct Wipeout {
    int victim = 0;
    int trapOwner = 0;
    TrapType trap = TrapType::Hammer;
};

struct RoundInput {
    std::set<int> players;            // everyone who ran this round
    std::set<int> finishers;          // reached the gate
    std::vector<Wipeout> wipeouts;
    std::map<int, double> ratingPoints; // rating averages earned this round
    std::set<int> absentOwners;       // left the match; their traps still score
};

// True when trap points are voided: every runner or no runner finished.
bool trapPointsVoided(const RoundInput& round);

// Points per player for one round. Every player gets an entry (0 if nothing).
// Throws std::invalid_argument when a finisher, victim, trap owner or rated
// player is not in `players` (a trap owner may be in `absentOwners`), or when an
// absent owner is also in `players`. A player wiped out by their own trap scores nothing.
std::map<int, double> scoreRound(const RoundInput& round);

class MatchScore {
public:
    // Scores the round, adds it to the totals and returns the round's points.
    std::map<int, double> addRound(const RoundInput& round);
    // Premiere rating averages for the round just played (doesn't count a round).
    void addRatings(const std::map<int, double>& points);
    double total(int player) const;
    const std::map<int, double>& totals() const { return totals_; }
    int roundsPlayed() const { return rounds_; }
    // Players by total, highest first; ties by lower player id.
    std::vector<int> podium() const;

private:
    std::map<int, double> totals_;
    int rounds_ = 0;
};

} // namespace tta
