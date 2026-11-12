#include "tta/EditList.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace tta {

namespace {

constexpr std::uint32_t kLeadMs = 1500;  // auto-edit: footage before an event
constexpr std::uint32_t kTailMs = 1500;  // and after it
constexpr double kAutoSlowMo = 0.5;
constexpr double kAutoZoom = 1.5;
constexpr std::uint32_t kAutoCardMs = 1000;

void checkIndex(int index, std::size_t size, const char* what) {
    if (index < 0 || static_cast<std::size_t>(index) >= size) {
        throw std::out_of_range(std::string(what) + " index " + std::to_string(index) +
                                " out of range (size " + std::to_string(size) + ")");
    }
}

bool dramatic(ReplayEventType type) {
    return type == ReplayEventType::Wipeout || type == ReplayEventType::Smite;
}

} // namespace

std::uint32_t Clip::durationMs() const {
    return static_cast<std::uint32_t>(std::lround((endMs - startMs) / speed));
}

std::uint32_t EditList::durationMs() const {
    std::uint32_t total = 0;
    for (const Clip& clip : clips_) {
        total += clip.durationMs();
    }
    return total;
}

void EditList::validate() const {
    for (const Clip& clip : clips_) {
        if (clip.endMs <= clip.startMs) {
            throw std::invalid_argument("clip must have endMs > startMs");
        }
        if (!(clip.speed >= kMinSpeed && clip.speed <= kMaxSpeed)) {
            throw std::invalid_argument("clip speed out of range: " + std::to_string(clip.speed));
        }
        if (!(clip.zoom >= kMinZoom && clip.zoom <= kMaxZoom)) {
            throw std::invalid_argument("clip zoom out of range: " + std::to_string(clip.zoom));
        }
    }
    const std::uint32_t total = durationMs();
    if (total > kMaxCutMs) {
        throw std::invalid_argument("cut is " + std::to_string(total) + " ms, cap is " +
                                    std::to_string(kMaxCutMs) + " ms");
    }
    for (const Overlay& overlay : overlays_) {
        if (overlay.durationMs == 0 || overlay.atMs + overlay.durationMs > total) {
            throw std::invalid_argument("overlay must be non-empty and inside the cut");
        }
        if (overlay.type != OverlayType::WastedCard && overlay.content.empty()) {
            throw std::invalid_argument("sticker/caption overlay needs content");
        }
    }
}

void EditList::commit(EditList next) {
    next.validate();
    *this = std::move(next);
}

void EditList::addClip(const Clip& clip) {
    EditList next = *this;
    next.clips_.push_back(clip);
    commit(std::move(next));
}

void EditList::removeClip(int index) {
    checkIndex(index, clips_.size(), "clip");
    EditList next = *this;
    next.clips_.erase(next.clips_.begin() + index);
    commit(std::move(next));
}

void EditList::trim(int index, std::uint32_t startMs, std::uint32_t endMs) {
    checkIndex(index, clips_.size(), "clip");
    EditList next = *this;
    next.clips_[index].startMs = startMs;
    next.clips_[index].endMs = endMs;
    commit(std::move(next));
}

void EditList::move(int from, int to) {
    checkIndex(from, clips_.size(), "clip");
    checkIndex(to, clips_.size(), "clip");
    EditList next = *this;
    const Clip clip = next.clips_[from];
    next.clips_.erase(next.clips_.begin() + from);
    next.clips_.insert(next.clips_.begin() + to, clip);
    commit(std::move(next));
}

void EditList::split(int index, std::uint32_t atReplayMs) {
    checkIndex(index, clips_.size(), "clip");
    const Clip& clip = clips_[index];
    if (atReplayMs <= clip.startMs || atReplayMs >= clip.endMs) {
        throw std::invalid_argument("split point must be inside the clip");
    }
    EditList next = *this;
    Clip tail = clip;
    tail.startMs = atReplayMs;
    next.clips_[index].endMs = atReplayMs;
    next.clips_.insert(next.clips_.begin() + index + 1, tail);
    commit(std::move(next));
}

void EditList::setCamera(int index, int camera) {
    checkIndex(index, clips_.size(), "clip");
    EditList next = *this;
    next.clips_[index].camera = camera;
    commit(std::move(next));
}

void EditList::setSpeed(int index, double speed) {
    checkIndex(index, clips_.size(), "clip");
    EditList next = *this;
    next.clips_[index].speed = speed;
    commit(std::move(next));
}

void EditList::setZoom(int index, double zoom) {
    checkIndex(index, clips_.size(), "clip");
    EditList next = *this;
    next.clips_[index].zoom = zoom;
    commit(std::move(next));
}

void EditList::addOverlay(const Overlay& overlay) {
    EditList next = *this;
    next.overlays_.push_back(overlay);
    commit(std::move(next));
}

void EditList::removeOverlay(int index) {
    checkIndex(index, overlays_.size(), "overlay");
    EditList next = *this;
    next.overlays_.erase(next.overlays_.begin() + index);
    commit(std::move(next));
}

EditList autoEdit(const Replay& replay, const std::vector<int>& cameras, int heavenCam) {
    if (cameras.empty()) {
        throw std::invalid_argument("autoEdit needs at least one camera");
    }
    std::vector<ReplayEvent> events = replay.events();
    std::stable_sort(events.begin(), events.end(),
                     [](const ReplayEvent& a, const ReplayEvent& b) { return a.timeMs < b.timeMs; });
    const auto endOfReplay =
        static_cast<std::uint32_t>(std::lround(replay.durationSeconds() * 1000.0));

    struct Window {
        std::uint32_t start, end;
        bool smite, dramatic;
    };
    std::vector<Window> windows;
    for (const ReplayEvent& event : events) {
        const std::uint32_t start = event.timeMs > kLeadMs ? event.timeMs - kLeadMs : 0;
        const std::uint32_t end = std::min(event.timeMs + kTailMs, endOfReplay);
        if (end <= start) {
            continue;
        }
        const bool smite = event.type == ReplayEventType::Smite;
        if (!windows.empty() && start <= windows.back().end) {
            Window& last = windows.back();
            last.end = std::max(last.end, end);
            last.smite = last.smite || smite;
            last.dramatic = last.dramatic || dramatic(event.type);
        } else {
            windows.push_back({start, end, smite, dramatic(event.type)});
        }
    }

    EditList cut;
    std::size_t nextCamera = 0;
    for (const Window& window : windows) {
        Clip clip;
        clip.camera = window.smite ? heavenCam : cameras[nextCamera++ % cameras.size()];
        clip.startMs = window.start;
        clip.endMs = window.end;
        clip.speed = window.dramatic ? kAutoSlowMo : 1.0;
        clip.zoom = window.dramatic ? kAutoZoom : 1.0;
        const std::uint32_t room = kMaxCutMs - cut.durationMs();
        if (clip.durationMs() > room) {
            clip.endMs = clip.startMs +
                         static_cast<std::uint32_t>(std::floor(room * clip.speed));
            if (clip.endMs <= clip.startMs) {
                break;
            }
        }
        cut.addClip(clip);
        if (window.dramatic) {
            const std::uint32_t total = cut.durationMs();
            const std::uint32_t card = std::min(kAutoCardMs, clip.durationMs());
            cut.addOverlay({OverlayType::WastedCard, total - card, card, ""});
        }
        if (cut.durationMs() >= kMaxCutMs) {
            break;
        }
    }
    return cut;
}

} // namespace tta
