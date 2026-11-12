#pragma once

#include "tta/Placement.h"
#include "tta/Replay.h"
#include "tta/Scoring.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace tta {

// Run-phase recording clock (plan "Recording"): turns variable frame deltas
// into 20 Hz replay frames, stamps trap hits with the trap's id and owner, and
// smites every unfinished runner once when the run timer runs out.
class RunRecorder {
public:
    // players: everyone in the round (runners and trap owners). Throws
    // std::invalid_argument on a non-positive run length or a bad player list.
    RunRecorder(std::vector<int> players, double runSeconds);

    // Advances the run clock; returns how many 20 Hz frames are now due.
    // The caller passes one addFrame() per due frame. Throws on a negative delta.
    int advance(double deltaSeconds);
    void addFrame(const std::vector<ReplayPose>& poses) { replay_.addFrame(poses); }

    // Hit event at the current time. Throws when `trap` is a camera.
    void recordHit(const Placement& trap, int victim);

    bool timedOut() const { return elapsedMs_ >= runMs_; }
    // Once timed out: records a smite at the run's end for each unfinished
    // runner and returns them. Returns nothing before the timeout or the second time.
    std::vector<int> takeSmites(const std::vector<int>& unfinished);

    // The round for scoring: every recorded hit is a wipeout paid to the trap's
    // owner; smites score nothing. Ratings are added later (Premiere).
    RoundInput roundInput(const std::set<int>& finishers) const;

    std::uint32_t elapsedMs() const { return elapsedMs_; }
    const Replay& replay() const { return replay_; }

private:
    Replay replay_;
    std::uint32_t runMs_;
    std::uint32_t elapsedMs_ = 0;
    double elapsedSeconds_ = 0.0;
    int framesDue_ = 0;
    bool smitten_ = false;
    std::map<int, TrapType> trapTypes_; // placement id -> type, from recorded hits
};

} // namespace tta
