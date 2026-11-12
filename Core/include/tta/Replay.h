#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace tta {

// Replay data (plan req 5): root + 15 bones per player, sampled at 20 Hz,
// plus hit / wipeout / smite events. Cameras render from this, not video.
constexpr int kReplayBones = 16;   // root + 15 bones
constexpr int kReplayHz = 20;
constexpr int kReplayFrameMs = 1000 / kReplayHz;
constexpr int kNoTrap = -1;        // trapId / trapOwner for a smite

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
};

// World-space transform, metres.
struct BoneTransform {
    Vec3 position;
    Quat rotation;
};

using ReplayPose = std::array<BoneTransform, kReplayBones>;

enum class ReplayEventType : std::uint8_t { Hit, Wipeout, Smite };

struct ReplayEvent {
    ReplayEventType type = ReplayEventType::Hit;
    std::uint32_t timeMs = 0;
    int victim = 0;
    int trapId = kNoTrap;    // placement id from Layout
    int trapOwner = kNoTrap; // player who placed the trap
};

// One round's recording for a fixed set of players (players join only between rounds).
class Replay {
public:
    // Throws std::invalid_argument on an empty or duplicated player list.
    explicit Replay(std::vector<int> players);

    const std::vector<int>& players() const { return players_; }
    int frameCount() const { return frameCount_; }
    double durationSeconds() const;

    // One 20 Hz tick: a pose per player, in players() order. Throws on a size mismatch.
    void addFrame(const std::vector<ReplayPose>& poses);
    // Throws when the victim isn't a player, a trap event has no trap/owner or
    // the owner isn't a player, or a smite carries a trap.
    void addEvent(const ReplayEvent& event);
    const std::vector<ReplayEvent>& events() const { return events_; }

    // Recorded sample, exactly as stored.
    const ReplayPose& frame(int player, int index) const;
    // Interpolated pose at tSeconds (clamped to the recording): Catmull-Rom on
    // positions, normalized lerp on rotations. Throws for an unknown player or no frames.
    ReplayPose sample(int player, double tSeconds) const;

    // Little-endian binary: float32 positions, int16 rotations.
    std::vector<std::uint8_t> serialize() const;

private:
    int playerIndex(int player) const;

    std::vector<int> players_;
    int frameCount_ = 0;
    std::vector<ReplayPose> poses_; // frame-major, players() order
    std::vector<ReplayEvent> events_;
};

// Throws std::invalid_argument on bad magic, version, truncation or trailing bytes.
Replay deserializeReplay(const std::vector<std::uint8_t>& bytes);

} // namespace tta
