#include "tta/ScoreBoard.h"

#include "tta/Scoring.h"

#include <algorithm>

namespace tta {

namespace {
std::map<int, int> ranks(const std::map<int, double>& points) {
    std::map<int, int> out;
    const std::vector<int> order = rankOrder(points);
    for (std::size_t i = 0; i < order.size(); ++i) out[order[i]] = static_cast<int>(i);
    return out;
}
} // namespace

std::vector<ScoreRow> scoreBoard(const std::map<int, double>& before, const std::map<int, double>& after) {
    std::map<int, double> from = before, to = after;
    for (const auto& [p, v] : after) from.emplace(p, 0.0);
    for (const auto& [p, v] : before) to.emplace(p, 0.0);
    const std::map<int, int> fromRank = ranks(from), toRank = ranks(to);
    std::vector<ScoreRow> rows;
    for (int p : rankOrder(to)) rows.push_back({p, from.at(p), to.at(p), fromRank.at(p), toRank.at(p)});
    return rows;
}

ScoreBar scoreBarAt(const ScoreRow& row, double t) {
    t = std::clamp(t, 0.0, 1.0);
    const double e = t * t * (3.0 - 2.0 * t); // smoothstep
    return {row.from + (row.to - row.from) * e, row.fromRank + (row.toRank - row.fromRank) * e};
}

} // namespace tta
