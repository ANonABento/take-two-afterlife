#include "doctest/doctest.h"
#include "tta/BotRace.h"

#include <cmath>
#include <stdexcept>

using namespace tta;

namespace {
RaceSense toward(RunnerSkill skill = RunnerSkill::Average) {
    RaceSense s;
    s.pos = {0, 0, 0};
    s.gate = {20, 0, 0};
    s.rightX = 0;
    s.rightY = 1;
    s.dt = 0.1;
    s.skill = skill;
    return s;
}
} // namespace

TEST_CASE("bot race runs at the gate and stops on it") {
    BotRace race(1);
    const RaceAction go = race.step(toward());
    CHECK(go.dirX > 0.9);
    CHECK(std::abs(go.dirY) < 0.05);
    CHECK_FALSE(go.jump);
    CHECK_FALSE(go.punch);
    RaceSense there = toward();
    there.pos = {19.7, 0, 0};
    const RaceAction done = race.step(there);
    CHECK(done.done);
    CHECK(done.dirX == 0.0);
}

TEST_CASE("bot race dodges a trap it can see and clumsy bots notice late") {
    BotRace average(1);
    RaceSense blocked = toward();
    blocked.hazards.push_back({2, 0, 0});
    const RaceAction dodge = average.step(blocked);
    CHECK(dodge.dirX > 0.0);
    CHECK(std::abs(dodge.dirY) > 0.3);

    BotRace clumsy(1);
    RaceSense late = toward(RunnerSkill::Clumsy);
    late.hazards.push_back({2, 0, 0});
    CHECK(std::abs(clumsy.step(late).dirY) < 0.05);

    BotRace pro(1);
    RaceSense wide = toward(RunnerSkill::Pro);
    wide.hazards.push_back({3.5, 1.0, 0});
    CHECK(pro.step(wide).dirY < 0.0);
}

TEST_CASE("bot race steers back onto floor and jumps a real gap") {
    BotRace race(1);
    RaceSense edge = toward();
    edge.groundCenter = false;
    edge.groundRight = false;
    const RaceAction back = race.step(edge);
    CHECK(back.dirY < 0.0);
    CHECK_FALSE(back.jump);

    RaceSense gap = toward();
    gap.groundLeft = gap.groundCenter = gap.groundRight = false;
    CHECK(race.step(gap).jump);
    gap.refuseJump = true;
    CHECK_FALSE(race.step(gap).jump);

    RaceSense step = toward();
    step.gate.z = 1.0;
    CHECK(race.step(step).jump);
}

TEST_CASE("bot race punches a rival in front and chases one ahead") {
    BotRace pro(1);
    RaceSense swing = toward(RunnerSkill::Pro);
    swing.punchReady = true;
    swing.rivals.push_back({1.0, 0, 0});
    CHECK(pro.step(swing).punch);
    swing.punchReady = false;
    CHECK_FALSE(pro.step(swing).punch);

    BotRace clumsy(1);
    RaceSense wild = toward(RunnerSkill::Clumsy);
    wild.punchReady = true;
    wild.rivals.push_back({0, 2.0, 0});
    CHECK(clumsy.step(wild).punch);

    BotRace average(1);
    RaceSense beside = toward();
    beside.punchReady = true;
    beside.rivals.push_back({0, 1.5, 0});
    CHECK_FALSE(average.step(beside).punch);

    RaceSense chase = toward();
    chase.rivals.push_back({3, 2, 0});
    CHECK(average.step(chase).dirY > 0.0);
    RaceSense line = toward(RunnerSkill::Clumsy);
    line.rivals.push_back({3, 2, 0});
    CHECK(std::abs(clumsy.step(line).dirY) < 0.15);
}

TEST_CASE("bot race sidesteps a stall and then asks to respawn") {
    BotRace race(1);
    RaceSense stuck = toward();
    bool sidestep = false;
    bool respawn = false;
    for (int i = 0; i < 14; ++i) sidestep |= std::abs(race.step(stuck).dirY) > 0.2;
    CHECK_FALSE(sidestep);
    for (int i = 0; i < 2; ++i) sidestep |= std::abs(race.step(stuck).dirY) > 0.2;
    CHECK(sidestep);
    for (int i = 0; i < 24; ++i) respawn |= race.step(stuck).respawn;
    CHECK_FALSE(respawn);
    CHECK(race.step(stuck).respawn);
    race.restart();
    CHECK_FALSE(race.step(stuck).respawn);
}

TEST_CASE("bot race rejects a negative dt") {
    BotRace race(1);
    RaceSense bad = toward();
    bad.dt = -0.1;
    CHECK_THROWS_AS(race.step(bad), std::invalid_argument);
}
