#include "tta/Ratings.h"

#include <stdexcept>
#include <string>

namespace tta {

namespace {
void requireScore(int score, const char* axis) {
    if (score < kMinRating || score > kMaxRating)
        throw std::invalid_argument(std::string(axis) + " rating " + std::to_string(score) + " is outside 1-5");
}

double average(const std::map<int, int>& scores) {
    double sum = 0.0;
    for (const auto& [rater, s] : scores) sum += s;
    return sum / static_cast<double>(scores.size());
}
} // namespace

void RatingBook::openCut(int cutId, const Cut& cut) {
    if (!cuts_.emplace(cutId, Entry{cut, {}, {}}).second)
        throw std::invalid_argument("cut " + std::to_string(cutId) + " is already open");
}

void RatingBook::rate(int cutId, int rater, int funny, int evil) {
    auto it = cuts_.find(cutId);
    if (it == cuts_.end()) throw std::invalid_argument("cut " + std::to_string(cutId) + " is not open");
    Entry& e = it->second;
    if (rater == e.cut.editor || rater == e.cut.builder)
        throw std::invalid_argument("player " + std::to_string(rater) + " cannot rate their own cut");
    if (e.funny.count(rater) != 0)
        throw std::invalid_argument("player " + std::to_string(rater) + " already rated cut " + std::to_string(cutId));
    requireScore(funny, "funny");
    requireScore(evil, "evil");
    e.funny[rater] = funny;
    e.evil[rater] = evil;
}

std::map<int, double> RatingBook::ratingPoints() const {
    std::map<int, double> points;
    for (const auto& [id, e] : cuts_) {
        if (e.funny.empty()) continue;
        points[e.cut.editor] += average(e.funny);
        points[e.cut.builder] += average(e.evil);
    }
    return points;
}

} // namespace tta
