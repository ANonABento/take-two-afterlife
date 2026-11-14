#pragma once

#include "tta/MatchSettings.h"
#include "tta/Ratings.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tta {

enum class PremiereStep { Intro, Playback, Rating, Reveal, Podium, Done };

struct PremiereCut {
    int cutId = 0;
    Cut cut;            // editor = director, builder = owner of the trap that got the victim
    int victim = 0;
    std::uint32_t durationMs = 0;
};

// One round's premiere: cuts in a seeded random order, each going
// Intro -> Playback -> Rating -> Reveal; then Podium on the final round, else Done.
// Intro/Rating/Reveal lengths come from `timings`; Playback lasts the cut's own length.
class Premiere {
public:
    // Throws std::invalid_argument on an empty list or a repeated cut id.
    Premiere(std::vector<PremiereCut> cuts, std::uint32_t seed, bool finalRound, const MatchSettings& timings = {});

    // Moves the clock; may cross several steps. No-op on Podium / Done.
    void advance(std::uint32_t ms);
    PremiereStep step() const { return step_; }
    std::uint32_t stepElapsedMs() const { return elapsedMs_; }
    // Length of the current step; 0 for Podium / Done (untimed).
    std::uint32_t stepLengthMs() const;
    // Index into order(); equals order().size() on Podium / Done.
    std::size_t index() const { return index_; }
    // Throws std::out_of_range on Podium / Done.
    const PremiereCut& current() const { return order_.at(index_); }
    const std::vector<PremiereCut>& order() const { return order_; }

    // Only during Rating, else std::logic_error. RatingBook rules apply.
    void rate(int rater, int funny, int evil);
    const RatingBook& ratings() const { return ratings_; }

private:
    void next();

    std::vector<PremiereCut> order_;
    RatingBook ratings_;
    bool finalRound_;
    std::uint32_t introMs_;
    std::uint32_t ratingMs_;
    std::uint32_t revealMs_;
    std::size_t index_ = 0;
    PremiereStep step_ = PremiereStep::Intro;
    std::uint32_t elapsedMs_ = 0;
};

} // namespace tta
