#pragma once

#include <map>
#include <vector>

namespace tta {

// Kahoot-style score board shown after each cut's Funny/Evil vote (the premiere's
// Reveal step): bars slide from the old totals to the new ones and re-sort by rank.
struct ScoreRow {
    int player = 0;
    double from = 0;  // total before this cut's votes
    double to = 0;    // total after
    int fromRank = 0; // 0 = top, by rankOrder()
    int toRank = 0;
};

// One row per player in either map (missing = 0), ordered by the new rank.
std::vector<ScoreRow> scoreBoard(const std::map<int, double>& before, const std::map<int, double>& after);

// Where a row's bar is at `t` (0..1 of the reveal, clamped), eased: its value and its
// slot, which moves from fromRank to toRank so rows swap places as the bars slide.
struct ScoreBar {
    double value = 0;
    double slot = 0;
};
ScoreBar scoreBarAt(const ScoreRow& row, double t);

} // namespace tta
