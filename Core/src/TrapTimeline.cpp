#include "tta/TrapTimeline.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace tta {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kHammerMaxDeg = 60.0;
constexpr double kCannonBallSpeed = 12.0;   // m/s
constexpr std::int64_t kCannonBallLifeMs = 2000;
constexpr double kPusherReachPerForce = 0.4; // m per force step
constexpr std::int64_t kTrapdoorMinClosedMs = 1000;

void checkSettings(TrapType type, const std::array<int, 2>& settings) {
    const TrapSpec& spec = trapSpec(type);
    for (int i = 0; i < 2; ++i) {
        const SettingSpec& s = spec.settings[i];
        if (settings[i] < s.min || settings[i] > s.max)
            throw std::invalid_argument(std::string(spec.name) + "." + s.name + " out of range: " + std::to_string(settings[i]));
    }
}
} // namespace

bool TrapPose::operator==(const TrapPose& o) const {
    return angleDeg == o.angleDeg && extensionMeters == o.extensionMeters && openFraction == o.openFraction &&
           projectileActive == o.projectileActive && projectileDistanceMeters == o.projectileDistanceMeters &&
           projectileRadiusMeters == o.projectileRadiusMeters && count == o.count;
}

std::int64_t trapPeriodMs(TrapType type, const std::array<int, 2>& s) {
    checkSettings(type, s);
    switch (type) {
    case TrapType::Hammer: return 6000 / s[0];
    case TrapType::Cannon: return 6000 / s[0];
    case TrapType::Spinner: return 6000 / s[0];
    case TrapType::PusherWall: return 1000LL * s[1];
    case TrapType::Trapdoor: return kTrapdoorMinClosedMs + 1000LL * s[0] + 1000LL * s[1];
    case TrapType::Banana: return 0;
    }
    throw std::invalid_argument("unknown trap type");
}

TrapPose pose(TrapType type, const std::array<int, 2>& s, double tSeconds) {
    if (!std::isfinite(tSeconds)) throw std::invalid_argument("trap time must be finite");
    const std::int64_t period = trapPeriodMs(type, s);
    const std::int64_t ms = std::llround(tSeconds * 1000.0);
    const std::int64_t phase = period == 0 ? 0 : ((ms % period) + period) % period;
    const double u = period == 0 ? 0.0 : static_cast<double>(phase) / static_cast<double>(period);
    TrapPose p;
    switch (type) {
    case TrapType::Hammer:
        p.angleDeg = (s[1] == 0 ? 1.0 : -1.0) * kHammerMaxDeg * std::cos(2.0 * kPi * u);
        break;
    case TrapType::Cannon:
        p.projectileRadiusMeters = 0.15 * s[1];
        p.projectileActive = phase < kCannonBallLifeMs;
        if (p.projectileActive) p.projectileDistanceMeters = kCannonBallSpeed * static_cast<double>(phase) / 1000.0;
        break;
    case TrapType::Spinner:
        p.angleDeg = 360.0 * u;
        p.extensionMeters = 0.5 + 0.5 * s[1];
        break;
    case TrapType::PusherWall: {
        // Push out, hold, pull back, rest: a quarter of the cycle each.
        const double reach = kPusherReachPerForce * s[0];
        const double q = u * 4.0;
        p.extensionMeters = q < 1.0 ? reach * q : q < 2.0 ? reach : q < 3.0 ? reach * (3.0 - q) : 0.0;
        break;
    }
    case TrapType::Trapdoor: {
        const bool open = phase >= kTrapdoorMinClosedMs + 1000LL * s[0];
        p.openFraction = open ? 1.0 : 0.0;
        p.angleDeg = open ? 90.0 : 0.0;
        break;
    }
    case TrapType::Banana:
        p.extensionMeters = static_cast<double>(s[0]);
        p.count = s[1];
        break;
    }
    return p;
}

} // namespace tta
