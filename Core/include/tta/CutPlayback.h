#pragma once

#include "tta/EditList.h"

#include <cstdint>
#include <optional>
#include <string>

namespace tta {

// What a cut shows at one instant of its own timeline: the editor preview and the
// cinema both play a cut by asking this every frame.
struct CutFrame {
    int clip = 0;
    int camera = 0;
    double replaySeconds = 0; // where to sample the replay
    double zoom = 1.0;
};

// nullopt at or after the cut's end (durationMs()), so a player stops exactly there.
std::optional<CutFrame> cutFrameAt(const EditList& cut, std::uint32_t cutMs);

// Reels autosave: a small text file, one line per clip / overlay. Captions may not
// contain line breaks. Loading replays every op through EditList, so a bad or
// over-cap file throws std::invalid_argument instead of loading.
std::string saveCut(const EditList& cut);
EditList loadCut(const std::string& text);

} // namespace tta
