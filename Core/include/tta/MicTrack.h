#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tta {

// Runner mic on the replay timeline (plan req 10): 16-bit mono PCM where
// sample 0 is replay time 0, so playback at replay time t reads sample t*rate.
constexpr int kMicSampleRate = 16000;
// Chunks whose stamp disagrees with the samples already written by more than
// this get re-aligned (silence padded or audio dropped), so drift stays bounded.
constexpr double kMicMaxDriftMs = 20.0;

class MicTrack {
public:
    // Throws std::invalid_argument on a non-positive sample rate.
    explicit MicTrack(int sampleRate = kMicSampleRate);

    // A captured chunk whose last sample was captured at replay time endMs.
    void append(const std::vector<std::int16_t>& pcm, double endMs);

    // `count` samples starting at replay time tMs; zero outside the recording
    // (silent fallback).
    std::vector<std::int16_t> read(double tMs, std::size_t count) const;

    // Replay time of the first sample at or after fromMs whose magnitude
    // reaches threshold.
    std::optional<double> onsetMs(double fromMs, std::int16_t threshold) const;

    double durationMs() const;
    int sampleRate() const { return rate_; }
    const std::vector<std::int16_t>& samples() const { return pcm_; }

    // RIFF/WAVE, PCM 16-bit mono. fromWav throws std::invalid_argument on
    // anything else.
    std::vector<std::uint8_t> toWav() const;
    static MicTrack fromWav(const std::vector<std::uint8_t>& bytes);

private:
    int rate_;
    std::vector<std::int16_t> pcm_;
};

} // namespace tta
