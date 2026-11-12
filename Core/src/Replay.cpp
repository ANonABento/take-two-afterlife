#include "tta/Replay.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace tta {
namespace {

constexpr std::uint8_t kMagic[4] = {'T', 'T', 'A', 'R'};
constexpr std::uint16_t kVersion = 1;
constexpr float kQuatScale = 32767.0f;

Quat normalized(Quat q) {
    const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (!(len > 0.0f)) {
        throw std::invalid_argument("replay rotation must be a non-zero quaternion");
    }
    return {q.x / len, q.y / len, q.z / len, q.w / len};
}

// Canonical form: unit length, w >= 0 (q and -q are the same rotation).
Quat canonical(Quat q) {
    q = normalized(q);
    if (q.w < 0.0f) {
        q = {-q.x, -q.y, -q.z, -q.w};
    }
    return q;
}

std::int16_t quantize(float v) {
    return static_cast<std::int16_t>(std::lround(std::clamp(v, -1.0f, 1.0f) * kQuatScale));
}

float catmullRom(float p0, float p1, float p2, float p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * (2.0f * p1 + (p2 - p0) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (3.0f * p1 - p0 - 3.0f * p2 + p3) * t3);
}

Quat nlerp(const Quat& a, Quat b, float t) {
    if (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w < 0.0f) {
        b = {-b.x, -b.y, -b.z, -b.w};
    }
    return normalized({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
                       a.w + (b.w - a.w) * t});
}

class Writer {
public:
    explicit Writer(std::vector<std::uint8_t>& out) : out_(out) {}
    void u8(std::uint8_t v) { out_.push_back(v); }
    void u16(std::uint16_t v) { uint(v, 2); }
    void u32(std::uint32_t v) { uint(v, 4); }
    void i32(int v) { u32(static_cast<std::uint32_t>(v)); }
    void i16(std::int16_t v) { u16(static_cast<std::uint16_t>(v)); }
    void f32(float v) {
        std::uint32_t bits;
        std::memcpy(&bits, &v, sizeof bits);
        u32(bits);
    }

private:
    void uint(std::uint32_t v, int bytes) {
        for (int i = 0; i < bytes; ++i) {
            out_.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
        }
    }
    std::vector<std::uint8_t>& out_;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& in) : in_(in) {}
    std::uint8_t u8() { return static_cast<std::uint8_t>(uint(1)); }
    std::uint16_t u16() { return static_cast<std::uint16_t>(uint(2)); }
    std::uint32_t u32() { return uint(4); }
    int i32() { return static_cast<int>(u32()); }
    std::int16_t i16() { return static_cast<std::int16_t>(u16()); }
    float f32() {
        const std::uint32_t bits = u32();
        float v;
        std::memcpy(&v, &bits, sizeof v);
        return v;
    }
    bool done() const { return pos_ == in_.size(); }

private:
    std::uint32_t uint(int bytes) {
        if (in_.size() - pos_ < static_cast<std::size_t>(bytes)) {
            throw std::invalid_argument("replay data truncated at byte " + std::to_string(pos_));
        }
        std::uint32_t v = 0;
        for (int i = 0; i < bytes; ++i) {
            v |= static_cast<std::uint32_t>(in_[pos_++]) << (8 * i);
        }
        return v;
    }
    const std::vector<std::uint8_t>& in_;
    std::size_t pos_ = 0;
};

} // namespace

Replay::Replay(std::vector<int> players) : players_(std::move(players)) {
    if (players_.empty()) {
        throw std::invalid_argument("replay needs at least one player");
    }
    std::vector<int> sorted = players_;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
        throw std::invalid_argument("replay player ids must be unique");
    }
}

double Replay::durationSeconds() const {
    return frameCount_ == 0 ? 0.0 : static_cast<double>(frameCount_ - 1) / kReplayHz;
}

int Replay::playerIndex(int player) const {
    const auto it = std::find(players_.begin(), players_.end(), player);
    if (it == players_.end()) {
        throw std::invalid_argument("player " + std::to_string(player) + " is not in this replay");
    }
    return static_cast<int>(it - players_.begin());
}

void Replay::addFrame(const std::vector<ReplayPose>& poses) {
    if (poses.size() != players_.size()) {
        throw std::invalid_argument("replay frame has " + std::to_string(poses.size()) +
                                    " poses for " + std::to_string(players_.size()) + " players");
    }
    for (ReplayPose pose : poses) {
        for (BoneTransform& bone : pose) {
            bone.rotation = canonical(bone.rotation);
        }
        poses_.push_back(pose);
    }
    ++frameCount_;
}

void Replay::addEvent(const ReplayEvent& event) {
    playerIndex(event.victim);
    if (event.type == ReplayEventType::Smite) {
        if (event.trapId != kNoTrap || event.trapOwner != kNoTrap) {
            throw std::invalid_argument("a smite has no trap or owner");
        }
    } else {
        if (event.trapId < 0) {
            throw std::invalid_argument("hit and wipeout events need a trap id");
        }
        playerIndex(event.trapOwner);
    }
    events_.push_back(event);
}

const ReplayPose& Replay::frame(int player, int index) const {
    if (index < 0 || index >= frameCount_) {
        throw std::out_of_range("replay frame " + std::to_string(index) + " out of range");
    }
    return poses_[static_cast<std::size_t>(index) * players_.size() +
                  static_cast<std::size_t>(playerIndex(player))];
}

ReplayPose Replay::sample(int player, double tSeconds) const {
    if (frameCount_ == 0) {
        throw std::logic_error("replay has no frames");
    }
    if (std::isnan(tSeconds)) {
        throw std::invalid_argument("replay sample time is NaN");
    }
    const double f = std::clamp(tSeconds * kReplayHz, 0.0, static_cast<double>(frameCount_ - 1));
    const int i1 = static_cast<int>(f);
    const int i2 = std::min(i1 + 1, frameCount_ - 1);
    const int i0 = std::max(i1 - 1, 0);
    const int i3 = std::min(i1 + 2, frameCount_ - 1);
    const float t = static_cast<float>(f - i1);
    const ReplayPose& p0 = frame(player, i0);
    const ReplayPose& p1 = frame(player, i1);
    const ReplayPose& p2 = frame(player, i2);
    const ReplayPose& p3 = frame(player, i3);

    ReplayPose out;
    for (int b = 0; b < kReplayBones; ++b) {
        const Vec3& a0 = p0[b].position;
        const Vec3& a1 = p1[b].position;
        const Vec3& a2 = p2[b].position;
        const Vec3& a3 = p3[b].position;
        // At the ends, extrapolate the missing neighbour linearly instead of repeating
        // the edge sample, which would flatten the tangent there.
        const Vec3 e0 = i0 == i1 ? Vec3{2 * a1.x - a2.x, 2 * a1.y - a2.y, 2 * a1.z - a2.z} : a0;
        const Vec3 e3 = i3 == i2 ? Vec3{2 * a2.x - a1.x, 2 * a2.y - a1.y, 2 * a2.z - a1.z} : a3;
        out[b].position = {catmullRom(e0.x, a1.x, a2.x, e3.x, t),
                           catmullRom(e0.y, a1.y, a2.y, e3.y, t),
                           catmullRom(e0.z, a1.z, a2.z, e3.z, t)};
        out[b].rotation = nlerp(p1[b].rotation, p2[b].rotation, t);
    }
    return out;
}

std::vector<std::uint8_t> Replay::serialize() const {
    std::vector<std::uint8_t> out;
    out.reserve(32 + poses_.size() * kReplayBones * 20 + events_.size() * 17);
    Writer w(out);
    for (std::uint8_t c : kMagic) {
        w.u8(c);
    }
    w.u16(kVersion);
    w.u8(kReplayBones);
    w.u8(kReplayHz);
    w.u32(static_cast<std::uint32_t>(players_.size()));
    for (int p : players_) {
        w.i32(p);
    }
    w.u32(static_cast<std::uint32_t>(frameCount_));
    for (const ReplayPose& pose : poses_) {
        for (const BoneTransform& bone : pose) {
            w.f32(bone.position.x);
            w.f32(bone.position.y);
            w.f32(bone.position.z);
            w.i16(quantize(bone.rotation.x));
            w.i16(quantize(bone.rotation.y));
            w.i16(quantize(bone.rotation.z));
            w.i16(quantize(bone.rotation.w));
        }
    }
    w.u32(static_cast<std::uint32_t>(events_.size()));
    for (const ReplayEvent& e : events_) {
        w.u8(static_cast<std::uint8_t>(e.type));
        w.u32(e.timeMs);
        w.i32(e.victim);
        w.i32(e.trapId);
        w.i32(e.trapOwner);
    }
    return out;
}

Replay deserializeReplay(const std::vector<std::uint8_t>& bytes) {
    Reader r(bytes);
    for (std::uint8_t c : kMagic) {
        if (r.u8() != c) {
            throw std::invalid_argument("not a replay file (bad magic)");
        }
    }
    const std::uint16_t version = r.u16();
    if (version != kVersion) {
        throw std::invalid_argument("unsupported replay version " + std::to_string(version));
    }
    if (r.u8() != kReplayBones || r.u8() != kReplayHz) {
        throw std::invalid_argument("replay bone count or rate doesn't match this build");
    }
    const std::uint32_t playerCount = r.u32();
    if (playerCount == 0 || playerCount > 64) {
        throw std::invalid_argument("replay player count out of range");
    }
    std::vector<int> players(playerCount);
    for (int& p : players) {
        p = r.i32();
    }
    Replay replay(players);
    const std::uint32_t frames = r.u32();
    std::vector<ReplayPose> poses(playerCount);
    for (std::uint32_t f = 0; f < frames; ++f) {
        for (ReplayPose& pose : poses) {
            for (BoneTransform& bone : pose) {
                bone.position.x = r.f32();
                bone.position.y = r.f32();
                bone.position.z = r.f32();
                bone.rotation.x = r.i16() / kQuatScale;
                bone.rotation.y = r.i16() / kQuatScale;
                bone.rotation.z = r.i16() / kQuatScale;
                bone.rotation.w = r.i16() / kQuatScale;
            }
        }
        replay.addFrame(poses);
    }
    const std::uint32_t eventCount = r.u32();
    for (std::uint32_t i = 0; i < eventCount; ++i) {
        ReplayEvent e;
        const std::uint8_t type = r.u8();
        if (type > static_cast<std::uint8_t>(ReplayEventType::Smite)) {
            throw std::invalid_argument("unknown replay event type " + std::to_string(type));
        }
        e.type = static_cast<ReplayEventType>(type);
        e.timeMs = r.u32();
        e.victim = r.i32();
        e.trapId = r.i32();
        e.trapOwner = r.i32();
        replay.addEvent(e);
    }
    if (!r.done()) {
        throw std::invalid_argument("replay data has trailing bytes");
    }
    return replay;
}

} // namespace tta
