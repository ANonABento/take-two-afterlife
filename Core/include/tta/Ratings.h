#pragma once

#include <map>

namespace tta {

// Premiere ratings (plan req 7).
constexpr int kMinRating = 1;
constexpr int kMaxRating = 5;

// One premiered cut: Funny ratings score its editor, Evil ratings its builder.
struct Cut {
    int editor = 0;
    int builder = 0;
};

class RatingBook {
public:
    // Throws std::invalid_argument if the cut id is already open.
    void openCut(int cutId, const Cut& cut);
    // Throws std::invalid_argument on an unknown cut, a score outside 1-5,
    // a rater who is the cut's editor or builder (self-rating), or a repeat rating.
    void rate(int cutId, int rater, int funny, int evil);
    // Per player: sum over their cuts of the average Funny (as editor) and
    // average Evil (as builder). Unrated cuts add nothing. Feeds RoundInput::ratingPoints.
    std::map<int, double> ratingPoints() const;

private:
    struct Entry {
        Cut cut;
        std::map<int, int> funny; // rater -> score
        std::map<int, int> evil;
    };
    std::map<int, Entry> cuts_;
};

} // namespace tta
