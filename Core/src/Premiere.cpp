#include "tta/Premiere.h"

#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace tta {

Premiere::Premiere(std::vector<PremiereCut> cuts, std::uint32_t seed, bool finalRound, const MatchSettings& timings)
    : order_(std::move(cuts)),
      finalRound_(finalRound),
      introMs_(static_cast<std::uint32_t>(timings.introSeconds) * 1000u),
      ratingMs_(static_cast<std::uint32_t>(timings.ratingSeconds) * 1000u),
      revealMs_(static_cast<std::uint32_t>(timings.revealSeconds) * 1000u) {
    if (order_.empty()) throw std::invalid_argument("premiere needs at least one cut");
    std::set<int> ids;
    for (const PremiereCut& c : order_) {
        if (!ids.insert(c.cutId).second) throw std::invalid_argument("cut " + std::to_string(c.cutId) + " listed twice");
        ratings_.openCut(c.cutId, c.cut);
    }
    // Own Fisher-Yates on mt19937 (fully specified), so every machine gets the same
    // order; std::shuffle's use of the engine differs between standard libraries.
    std::mt19937 rng(seed);
    for (std::size_t i = order_.size() - 1; i > 0; --i) std::swap(order_[i], order_[rng() % (i + 1)]);
}

std::uint32_t Premiere::stepLengthMs() const {
    switch (step_) {
    case PremiereStep::Intro: return introMs_;
    case PremiereStep::Playback: return current().durationMs;
    case PremiereStep::Rating: return ratingMs_;
    case PremiereStep::Reveal: return revealMs_;
    case PremiereStep::Podium:
    case PremiereStep::Done: break;
    }
    return 0;
}

void Premiere::advance(std::uint32_t ms) {
    elapsedMs_ += ms;
    while (step_ != PremiereStep::Podium && step_ != PremiereStep::Done && elapsedMs_ >= stepLengthMs()) {
        elapsedMs_ -= stepLengthMs();
        next();
    }
    if (step_ == PremiereStep::Podium || step_ == PremiereStep::Done) elapsedMs_ = 0;
}

void Premiere::next() {
    switch (step_) {
    case PremiereStep::Intro: step_ = PremiereStep::Playback; break;
    case PremiereStep::Playback: step_ = PremiereStep::Rating; break;
    case PremiereStep::Rating: step_ = PremiereStep::Reveal; break;
    case PremiereStep::Reveal:
        if (++index_ < order_.size()) step_ = PremiereStep::Intro;
        else step_ = finalRound_ ? PremiereStep::Podium : PremiereStep::Done;
        break;
    case PremiereStep::Podium:
    case PremiereStep::Done: break;
    }
}

void Premiere::rate(int rater, int funny, int evil) {
    if (step_ != PremiereStep::Rating) throw std::logic_error("ratings are only open during the rating screen");
    ratings_.rate(current().cutId, rater, funny, evil);
}

} // namespace tta
