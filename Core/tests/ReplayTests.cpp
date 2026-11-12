#include "tta/Replay.h"

#include <doctest/doctest.h>

#include <cmath>
#include <stdexcept>

using namespace tta;

namespace {

// Running ragdoll-ish motion: root runs at 6 m/s with a bob, limbs swing.
ReplayPose truePose(int player, double t) {
    ReplayPose pose;
    const double rootX = 6.0 * t;
    const double rootY = -3.0 + 2.0 * player + 0.5 * std::sin(0.8 * t);
    const double rootZ = 1.0 + 0.1 * std::sin(2.0 * M_PI * 3.0 * t);
    for (int b = 0; b < kReplayBones; ++b) {
        const double phase = 2.0 * M_PI * 1.5 * t + b;
        const double reach = b == 0 ? 0.0 : 0.3;
        pose[b].position = {static_cast<float>(rootX + reach * std::sin(phase)),
                            static_cast<float>(rootY + 0.05 * b),
                            static_cast<float>(rootZ + reach * std::cos(phase) - 0.04 * b)};
        const double half = 0.5 * std::sin(phase);
        pose[b].rotation = {0.0f, 0.0f, static_cast<float>(std::sin(half)),
                            static_cast<float>(std::cos(half))};
    }
    return pose;
}

Replay record(int players, double seconds) {
    std::vector<int> ids;
    for (int p = 0; p < players; ++p) {
        ids.push_back(p);
    }
    Replay replay(ids);
    const int frames = static_cast<int>(seconds * kReplayHz) + 1;
    std::vector<ReplayPose> poses(ids.size());
    for (int f = 0; f < frames; ++f) {
        for (int p = 0; p < players; ++p) {
            poses[p] = truePose(p, static_cast<double>(f) / kReplayHz);
        }
        replay.addFrame(poses);
    }
    return replay;
}

double distance(const Vec3& a, const Vec3& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

} // namespace

TEST_CASE("3-minute 4-player replay serializes under 8 MB") {
    Replay replay = record(4, 180.0);
    replay.addEvent({ReplayEventType::Wipeout, 1000, 1, 3, 2});
    CHECK(replay.frameCount() == 3601);
    const auto bytes = replay.serialize();
    MESSAGE("3 min, 4 players: " << bytes.size() << " bytes");
    CHECK(bytes.size() < 8u * 1024u * 1024u);
}

TEST_CASE("interpolation error between samples is under 1 cm") {
    const Replay replay = deserializeReplay(record(4, 10.0).serialize());
    double worst = 0.0;
    for (int p = 0; p < 4; ++p) {
        for (int step = 0; step < 10 * kReplayHz * 10; ++step) {
            const double t = step / (10.0 * kReplayHz) + 0.0013;
            if (t > replay.durationSeconds()) {
                break;
            }
            const ReplayPose got = replay.sample(p, t);
            const ReplayPose want = truePose(p, t);
            for (int b = 0; b < kReplayBones; ++b) {
                worst = std::max(worst, distance(got[b].position, want[b].position));
            }
        }
    }
    MESSAGE("worst interpolation error: " << worst * 100.0 << " cm");
    CHECK(worst < 0.01);
}

TEST_CASE("sample hits stored frames exactly and clamps outside the recording") {
    const Replay replay = record(2, 2.0);
    const ReplayPose at = replay.sample(1, 0.5);
    CHECK(at[3].position.x == replay.frame(1, 10)[3].position.x);
    CHECK(replay.sample(0, -5.0)[0].position.x == replay.frame(0, 0)[0].position.x);
    CHECK(replay.sample(0, 99.0)[0].position.x == replay.frame(0, 40)[0].position.x);
    CHECK_THROWS_AS(replay.sample(7, 0.0), std::invalid_argument);
    CHECK_THROWS_AS(Replay({1}).sample(1, 0.0), std::logic_error);
}

TEST_CASE("rotations are interpolated and stay unit length") {
    const Replay replay = record(1, 2.0);
    const Quat q = replay.sample(0, 0.525)[5].rotation;
    CHECK(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w == doctest::Approx(1.0).epsilon(1e-5));
    const Quat want = truePose(0, 0.525)[5].rotation;
    CHECK(std::fabs(q.z * want.z + q.w * want.w) > 0.999);
}

TEST_CASE("serialize round-trips frames and events") {
    Replay replay = record(3, 1.0);
    replay.addEvent({ReplayEventType::Hit, 250, 0, 4, 1});
    replay.addEvent({ReplayEventType::Wipeout, 300, 0, 4, 1});
    replay.addEvent({ReplayEventType::Smite, 900, 2, kNoTrap, kNoTrap});
    const Replay back = deserializeReplay(replay.serialize());
    CHECK(back.players() == replay.players());
    CHECK(back.frameCount() == replay.frameCount());
    for (int b = 0; b < kReplayBones; ++b) {
        const BoneTransform& a = replay.frame(2, 7)[b];
        const BoneTransform& c = back.frame(2, 7)[b];
        CHECK(a.position.x == c.position.x);
        CHECK(a.position.z == c.position.z);
        CHECK(c.rotation.w == doctest::Approx(a.rotation.w).epsilon(1e-4));
        CHECK(c.rotation.z == doctest::Approx(a.rotation.z).epsilon(1e-4));
    }
    REQUIRE(back.events().size() == 3);
    CHECK(back.events()[1].type == ReplayEventType::Wipeout);
    CHECK(back.events()[1].trapId == 4);
    CHECK(back.events()[1].trapOwner == 1);
    CHECK(back.events()[2].type == ReplayEventType::Smite);
    CHECK(back.events()[2].victim == 2);
}

TEST_CASE("bad input is rejected") {
    CHECK_THROWS_AS(Replay({}), std::invalid_argument);
    CHECK_THROWS_AS(Replay({1, 1}), std::invalid_argument);
    Replay replay({1, 2});
    CHECK_THROWS_AS(replay.addFrame({ReplayPose{}}), std::invalid_argument);
    CHECK_THROWS_AS(replay.addEvent({ReplayEventType::Hit, 0, 9, 0, 1}), std::invalid_argument);
    CHECK_THROWS_AS(replay.addEvent({ReplayEventType::Wipeout, 0, 1, kNoTrap, 2}), std::invalid_argument);
    CHECK_THROWS_AS(replay.addEvent({ReplayEventType::Wipeout, 0, 1, 0, 9}), std::invalid_argument);
    CHECK_THROWS_AS(replay.addEvent({ReplayEventType::Smite, 0, 1, 0, 2}), std::invalid_argument);
    ReplayPose zero;
    zero[0].rotation = {0, 0, 0, 0};
    CHECK_THROWS_AS(replay.addFrame({zero, ReplayPose{}}), std::invalid_argument);

    auto bytes = record(1, 0.5).serialize();
    auto bad = bytes;
    bad[0] = 'X';
    CHECK_THROWS_AS(deserializeReplay(bad), std::invalid_argument);
    auto shortBytes = bytes;
    shortBytes.resize(bytes.size() - 3);
    CHECK_THROWS_AS(deserializeReplay(shortBytes), std::invalid_argument);
    auto longBytes = bytes;
    longBytes.push_back(0);
    CHECK_THROWS_AS(deserializeReplay(longBytes), std::invalid_argument);
}
