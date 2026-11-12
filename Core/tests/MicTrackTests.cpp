#include "doctest/doctest.h"
#include "tta/MicTrack.h"

#include <cmath>
#include <stdexcept>

using namespace tta;

namespace {
// Fixture: a click every second; the mic clock runs 0.5% fast and chunks are
// delivered in irregular batches, each stamped with the replay time at delivery.
MicTrack recordClicks(double seconds) {
    MicTrack track;
    const double micRate = kMicSampleRate * 1.005;
    long long micSample = 0;
    int batch = 0;
    std::vector<std::int16_t> pending;
    while (micSample / micRate < seconds) {
        for (int i = 0; i < 320; ++i, ++micSample) { // 20 ms chunks
            const double t = micSample / micRate;
            pending.push_back(t >= 1.0 && t - std::floor(t) < 0.005 ? 20000 : 0);
        }
        ++batch;
        if (batch % (1 + batch % 4) == 0) {
            track.append(pending, micSample / micRate * 1000.0);
            pending.clear();
        }
    }
    if (!pending.empty()) track.append(pending, micSample / micRate * 1000.0);
    return track;
}
} // namespace

TEST_CASE("clicks land on replay time within 50 ms despite drift and batching") {
    const MicTrack track = MicTrack::fromWav(recordClicks(30.0).toWav());
    for (int s = 1; s < 30; ++s) {
        const auto onset = track.onsetMs(s * 1000.0 - 400.0, 10000);
        REQUIRE(onset.has_value());
        CHECK(std::fabs(*onset - s * 1000.0) < 50.0);
    }
}

TEST_CASE("first chunk is placed at its replay time; gaps become silence") {
    MicTrack track;
    track.append(std::vector<std::int16_t>(160, 7), 1010.0); // 10 ms ending at 1010
    CHECK(track.samples().size() == 16160);
    CHECK(track.read(999.0, 1)[0] == 0);
    CHECK(track.read(1000.0, 1)[0] == 7);
    track.append(std::vector<std::int16_t>(160, 9), 2010.0); // 1 s gap
    CHECK(track.read(1500.0, 1)[0] == 0);
    CHECK(track.read(2005.0, 1)[0] == 9);
}

TEST_CASE("a chunk stamped behind the written audio drops its overlap") {
    MicTrack track;
    track.append(std::vector<std::int16_t>(16000, 1), 1000.0);
    track.append(std::vector<std::int16_t>(1600, 2), 1000.0); // all behind -> dropped
    CHECK(track.samples().size() == 16000);
    track.append(std::vector<std::int16_t>(1600, 3), 1050.0); // half overlaps
    CHECK(track.samples().size() == 16800);
    CHECK(track.read(1049.0, 1)[0] == 3);
}

TEST_CASE("reads outside the recording are silent") {
    MicTrack empty;
    for (std::int16_t s : empty.read(500.0, 64)) CHECK(s == 0);
    CHECK_FALSE(empty.onsetMs(0.0, 1).has_value());
}

TEST_CASE("WAVE round trip and rejects") {
    MicTrack track(8000);
    track.append({1, -2, 3, -32768}, 0.5);
    const MicTrack back = MicTrack::fromWav(track.toWav());
    CHECK(back.sampleRate() == 8000);
    CHECK(back.samples() == track.samples());
    CHECK_THROWS_AS(MicTrack::fromWav({'R', 'I', 'F', 'F'}), std::invalid_argument);
    CHECK_THROWS_AS(MicTrack(0), std::invalid_argument);
}
