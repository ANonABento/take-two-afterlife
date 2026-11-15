#include "doctest/doctest.h"
#include "tta/Punch.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("punch runs wind-up, active, recovery on exact edges") {
    Punch p;
    REQUIRE(p.start(LifeState::Alive));
    CHECK(p.phase() == PunchPhase::WindUp);
    CHECK(p.advance(0.44) == PunchEvent::None);
    CHECK_FALSE(p.hitWindowOpen());
    CHECK(p.advance(0.01) == PunchEvent::OpenHitWindow);
    CHECK(p.hitWindowOpen());
    CHECK(p.advance(0.09) == PunchEvent::None);
    CHECK(p.hitWindowOpen());
    CHECK(p.advance(0.01) == PunchEvent::CloseHitWindow);
    CHECK(p.phase() == PunchPhase::Recovery);
    CHECK(p.advance(0.34) == PunchEvent::None);
    CHECK(p.advance(0.01) == PunchEvent::Done);
    CHECK(p.phase() == PunchPhase::Idle);
}

TEST_CASE("punch phases end at exactly their length in one step") {
    Punch p;
    REQUIRE(p.start(LifeState::Alive));
    CHECK(p.advance(kPunchWindUpSeconds) == PunchEvent::OpenHitWindow);
    CHECK(p.advance(kPunchActiveSeconds) == PunchEvent::CloseHitWindow);
    CHECK(p.advance(kPunchRecoverySeconds) == PunchEvent::Done);
}

TEST_CASE("one big step moves only one phase") {
    Punch p;
    REQUIRE(p.start(LifeState::Alive));
    CHECK(p.advance(2.0) == PunchEvent::OpenHitWindow);
    CHECK(p.phase() == PunchPhase::Active);
}

TEST_CASE("cooldown runs 1.5 s from the press") {
    Punch p;
    REQUIRE(p.start(LifeState::Alive));
    CHECK_FALSE(p.start(LifeState::Alive));  // mid-punch
    p.advance(0.45);
    p.advance(0.10);
    p.advance(0.35);
    REQUIRE(p.phase() == PunchPhase::Idle);
    CHECK_FALSE(p.start(LifeState::Alive));  // 0.9 s in, still cooling
    p.advance(0.59);
    CHECK(p.cooldownLeft() > 0.0);
    CHECK_FALSE(p.start(LifeState::Alive));
    p.advance(0.01);
    CHECK(p.cooldownLeft() == 0.0);
    CHECK(p.start(LifeState::Alive));
}

TEST_CASE("can't punch unless alive") {
    for (LifeState s : {LifeState::Frozen, LifeState::Ragdoll, LifeState::Stunned}) {
        Punch p;
        CHECK_FALSE(p.start(s));
        CHECK(p.phase() == PunchPhase::Idle);
        CHECK(p.cooldownLeft() == 0.0);
    }
}

TEST_CASE("idle punch ignores time and rejects negative dt") {
    Punch p;
    CHECK(p.advance(5.0) == PunchEvent::None);
    CHECK_THROWS_AS(p.advance(-0.1), std::invalid_argument);
}
