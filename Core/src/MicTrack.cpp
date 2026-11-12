#include "tta/MicTrack.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace tta {

namespace {
void put32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}
void put16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
}
void putTag(std::vector<std::uint8_t>& out, const char* tag) {
    out.insert(out.end(), tag, tag + 4);
}
std::uint32_t get32(const std::vector<std::uint8_t>& in, std::size_t at) {
    return static_cast<std::uint32_t>(in[at]) | (static_cast<std::uint32_t>(in[at + 1]) << 8) |
           (static_cast<std::uint32_t>(in[at + 2]) << 16) | (static_cast<std::uint32_t>(in[at + 3]) << 24);
}
std::uint16_t get16(const std::vector<std::uint8_t>& in, std::size_t at) {
    return static_cast<std::uint16_t>(in[at] | (in[at + 1] << 8));
}
} // namespace

MicTrack::MicTrack(int sampleRate) : rate_(sampleRate) {
    if (sampleRate <= 0) throw std::invalid_argument("MicTrack: sample rate must be positive");
}

void MicTrack::append(const std::vector<std::int16_t>& pcm, double endMs) {
    const long long n = static_cast<long long>(pcm.size());
    const long long target = std::llround(endMs * rate_ / 1000.0) - n; // where this chunk should start
    const long long have = static_cast<long long>(pcm_.size());
    const long long slack = std::llround(kMicMaxDriftMs * rate_ / 1000.0);
    long long skip = 0;
    if (target > have + slack) {
        pcm_.resize(static_cast<std::size_t>(target), 0); // capture gap -> silence
    } else if (target < have - slack) {
        skip = have - target; // stamp says the chunk overlaps written audio: drop the overlap
    }
    if (skip >= n) return;
    pcm_.insert(pcm_.end(), pcm.begin() + skip, pcm.end());
}

std::vector<std::int16_t> MicTrack::read(double tMs, std::size_t count) const {
    std::vector<std::int16_t> out(count, 0);
    const long long first = std::llround(tMs * rate_ / 1000.0);
    for (std::size_t i = 0; i < count; ++i) {
        const long long at = first + static_cast<long long>(i);
        if (at >= 0 && at < static_cast<long long>(pcm_.size())) out[i] = pcm_[static_cast<std::size_t>(at)];
    }
    return out;
}

std::optional<double> MicTrack::onsetMs(double fromMs, std::int16_t threshold) const {
    const long long first = std::max(0LL, std::llround(fromMs * rate_ / 1000.0));
    for (std::size_t i = static_cast<std::size_t>(first); i < pcm_.size(); ++i) {
        if (std::abs(static_cast<int>(pcm_[i])) >= threshold) return i * 1000.0 / rate_;
    }
    return std::nullopt;
}

double MicTrack::durationMs() const { return pcm_.size() * 1000.0 / rate_; }

std::vector<std::uint8_t> MicTrack::toWav() const {
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(pcm_.size() * 2);
    std::vector<std::uint8_t> out;
    out.reserve(44 + dataBytes);
    putTag(out, "RIFF");
    put32(out, 36 + dataBytes);
    putTag(out, "WAVE");
    putTag(out, "fmt ");
    put32(out, 16);
    put16(out, 1); // PCM
    put16(out, 1); // mono
    put32(out, static_cast<std::uint32_t>(rate_));
    put32(out, static_cast<std::uint32_t>(rate_) * 2);
    put16(out, 2);
    put16(out, 16);
    putTag(out, "data");
    put32(out, dataBytes);
    for (std::int16_t s : pcm_) put16(out, static_cast<std::uint16_t>(s));
    return out;
}

MicTrack MicTrack::fromWav(const std::vector<std::uint8_t>& in) {
    auto tag = [&](std::size_t at, const char* t) { return at + 4 <= in.size() && std::memcmp(&in[at], t, 4) == 0; };
    if (!tag(0, "RIFF") || !tag(8, "WAVE")) throw std::invalid_argument("MicTrack: not a WAVE file");
    int rate = 0;
    std::size_t at = 12;
    while (at + 8 <= in.size()) {
        const std::uint32_t size = get32(in, at + 4);
        const std::size_t body = at + 8;
        if (body + size > in.size()) throw std::invalid_argument("MicTrack: truncated WAVE chunk");
        if (tag(at, "fmt ")) {
            if (size < 16 || get16(in, body) != 1 || get16(in, body + 2) != 1 || get16(in, body + 14) != 16)
                throw std::invalid_argument("MicTrack: WAVE must be PCM 16-bit mono");
            rate = static_cast<int>(get32(in, body + 4));
        } else if (tag(at, "data")) {
            if (rate == 0) throw std::invalid_argument("MicTrack: WAVE data before fmt");
            MicTrack track(rate);
            track.pcm_.resize(size / 2);
            for (std::size_t i = 0; i < track.pcm_.size(); ++i)
                track.pcm_[i] = static_cast<std::int16_t>(get16(in, body + 2 * i));
            return track;
        }
        at = body + size + (size & 1);
    }
    throw std::invalid_argument("MicTrack: WAVE has no data chunk");
}

} // namespace tta
