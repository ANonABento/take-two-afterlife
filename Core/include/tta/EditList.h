#pragma once

#include "tta/Replay.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tta {

// Edit list (plan req 6): a cut is an ordered list of clips rendered from replay
// data, plus overlays on the cut's own timeline. Every operation is validated on a
// copy and only committed when the cut stays valid, so a rejected edit changes nothing.
constexpr std::uint32_t kMaxCutMs = 30000; // 30 s cap per cut
constexpr double kMinSpeed = 0.25;          // slowest slow-mo
constexpr double kMaxSpeed = 1.0;
constexpr double kMinZoom = 1.0;
constexpr double kMaxZoom = 4.0;

// A camera's view of [startMs, endMs) of the replay.
struct Clip {
    int camera = 0;
    std::uint32_t startMs = 0;
    std::uint32_t endMs = 0;
    double speed = 1.0; // < 1 is slow-mo
    double zoom = 1.0;

    // Length on the cut's timeline.
    std::uint32_t durationMs() const;
};

enum class OverlayType : std::uint8_t { Sticker, Caption, WastedCard };

struct Overlay {
    OverlayType type = OverlayType::Caption;
    std::uint32_t atMs = 0;     // cut time
    std::uint32_t durationMs = 0;
    std::string content;        // sticker id or caption text; unused for WastedCard
};

class EditList {
public:
    const std::vector<Clip>& clips() const { return clips_; }
    const std::vector<Overlay>& overlays() const { return overlays_; }
    std::uint32_t durationMs() const;

    // All of these throw std::invalid_argument (std::out_of_range for a bad index)
    // when the result would be invalid or exceed kMaxCutMs.
    void addClip(const Clip& clip);
    void removeClip(int index);
    void trim(int index, std::uint32_t startMs, std::uint32_t endMs);
    void move(int from, int to);                     // reorder
    void split(int index, std::uint32_t atReplayMs); // cut point for an angle change
    void setCamera(int index, int camera);           // multi-angle
    void setSpeed(int index, double speed);          // slow-mo
    void setZoom(int index, double zoom);
    void addOverlay(const Overlay& overlay);
    void removeOverlay(int index);

private:
    void commit(EditList next);
    void validate() const;

    std::vector<Clip> clips_;
    std::vector<Overlay> overlays_;
};

// Fallback cut built from the replay's hit / wipeout / smite events, in time order.
// Each event gets a window around it on the next camera from `cameras` (round-robin);
// smites use `heavenCam`. Overlapping windows merge. Wipeouts and smites play in
// slow-mo with zoom and end on a WASTED card. Stops at kMaxCutMs, trimming the last
// clip to fit. Throws when `cameras` is empty.
EditList autoEdit(const Replay& replay, const std::vector<int>& cameras, int heavenCam);

} // namespace tta
