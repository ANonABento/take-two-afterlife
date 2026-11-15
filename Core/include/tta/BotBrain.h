#pragma once

#include "tta/Placement.h"
#include "tta/Ratings.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace tta {

// Engine-free bot decisions for Play Solo (MVP req 3). Same seed -> same choices on every platform:
// uses its own SplitMix64 stream, not <random> distributions (those differ between standard libraries).
enum class RunnerSkill { Clumsy, Average, Pro };

struct RunnerPlan {
    RunnerSkill skill = RunnerSkill::Average;
    double failChance = 0; // 0..1, chance per obstacle that the bot fails on purpose
};

class BotBrain {
public:
    BotBrain(int playerId, std::uint64_t seed);

    int playerId() const { return playerId_; }
    // Places traps and cameras for this bot into `layout` through Layout's own rules (budget, cap, bounds).
    void build(Layout& layout);
    RunnerPlan planRun();
    // Rates every cut in `cuts` (already opened in `book`) whose editor and builder are not this bot.
    void rate(RatingBook& book, const std::vector<std::pair<int, Cut>>& cuts);

private:
    std::uint64_t next();
    int range(int lo, int hi); // inclusive
    double unit();             // [0, 1)

    int playerId_;
    std::uint64_t state_;
};

} // namespace tta
