#include "tta/BotRace.h"

#include <cmath>
#include <stdexcept>

namespace tta {

namespace {
constexpr double kStepUpMeters = 0.4;

double lookAhead(RunnerSkill skill) {
    switch (skill) {
    case RunnerSkill::Clumsy: return 1.2;
    case RunnerSkill::Pro: return 4.0;
    case RunnerSkill::Average: return 2.5;
    }
    return 2.5;
}

double punchReach(RunnerSkill skill) {
    switch (skill) {
    case RunnerSkill::Clumsy: return 2.2;
    case RunnerSkill::Pro: return 1.3;
    case RunnerSkill::Average: return 1.6;
    }
    return 1.6;
}
} // namespace

BotRace::BotRace(std::uint64_t seed) : state_(seed ^ 0xA5A5A5A5A5A5A5A5ULL) {}

std::uint64_t BotRace::next() {
    std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void BotRace::restart() {
    haveBest_ = false;
    stalled_ = 0;
    sidestepping_ = false;
}

RaceAction BotRace::step(const RaceSense& in) {
    if (!(in.dt >= 0)) throw std::invalid_argument("BotRace::step dt must be >= 0");
    RaceAction out;
    const double dx = in.gate.x - in.pos.x;
    const double dy = in.gate.y - in.pos.y;
    const double dist = std::hypot(dx, dy);
    if (dist <= kGateMeters) {
        out.done = true;
        return out;
    }
    const double fwdX = dx / dist;
    const double fwdY = dy / dist;
    double ax = fwdX;
    double ay = fwdY;

    const double wall = kCorridorWidthMeters / 2.0 - 0.6;
    if (in.pos.y > wall) ay -= 1.0;
    if (in.pos.y < -wall) ay += 1.0;

    if (!in.groundCenter) {
        if (in.groundLeft && !in.groundRight) {
            ax -= in.rightX;
            ay -= in.rightY;
        } else if (in.groundRight && !in.groundLeft) {
            ax += in.rightX;
            ay += in.rightY;
        } else if (!in.refuseJump) {
            out.jump = true; // nothing under the nose, or a hole with floor on both sides
        }
    }
    if (in.gate.z - in.pos.z > kStepUpMeters) out.jump = true;

    const double look = lookAhead(in.skill);
    const RoutePoint* hazard = nullptr;
    double hazardDist = look;
    for (const RoutePoint& h : in.hazards) {
        const double hx = h.x - in.pos.x;
        if (hx < -0.5 || hx > look) continue;
        const double d = std::hypot(hx, h.y - in.pos.y);
        if (d < hazardDist) {
            hazardDist = d;
            hazard = &h;
        }
    }
    if (hazard) {
        if (std::abs(hazard->y - in.pos.y) < 0.25) ay += (in.pos.y > 0.0) ? -1.0 : 1.0;
        else ay += (in.pos.y >= hazard->y) ? 1.0 : -1.0;
    }

    const double reach = punchReach(in.skill);
    const RoutePoint* nearest = nullptr;
    double nearestDist = 0;
    for (const RoutePoint& rival : in.rivals) {
        const double rx = rival.x - in.pos.x;
        const double ry = rival.y - in.pos.y;
        const double rd = std::hypot(rx, ry);
        if (!nearest || rd < nearestDist) {
            nearest = &rival;
            nearestDist = rd;
        }
        // Contest someone ahead. Clumsy bots just run their line.
        if (in.skill != RunnerSkill::Clumsy && rx > 0.0 && rx < 5.0 && std::abs(ry) < 2.5) {
            ax += 0.15 * rx;
            ay += 0.45 * ry;
        }
    }
    if (nearest && in.punchReady && nearestDist <= reach && nearestDist > 0.05) {
        const double rx = nearest->x - in.pos.x;
        const double ry = nearest->y - in.pos.y;
        const double dot = (rx * fwdX + ry * fwdY) / nearestDist;
        bool lined = in.skill == RunnerSkill::Clumsy || dot > (in.skill == RunnerSkill::Pro ? 0.45 : 0.15);
        if (in.skill == RunnerSkill::Pro && !lined) {
            for (const RoutePoint& h : in.hazards) {
                if (std::hypot(h.x - nearest->x, h.y - nearest->y) < 2.0) lined = true;
            }
        }
        out.punch = lined;
    }

    const double along = -dist;
    if (!haveBest_ || along > bestAlong_ + kProgressMeters) {
        haveBest_ = true;
        bestAlong_ = along;
        stalled_ = 0;
        sidestepping_ = false;
    } else if ((stalled_ += in.dt) >= kSidestepSeconds) {
        if (!sidestepping_) {
            sidestep_ = (next() & 1ULL) ? 1 : -1;
            sidestepping_ = true;
        }
        ay += static_cast<double>(sidestep_) * 1.5;
        if (stalled_ >= kGiveUpSeconds) {
            out.respawn = true;
            stalled_ = 0;
            sidestepping_ = false;
        }
    }

    const double len = std::hypot(ax, ay);
    if (len > 1e-6) {
        out.dirX = ax / len;
        out.dirY = ay / len;
    }
    return out;
}

} // namespace tta
