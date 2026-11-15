#include "doctest/doctest.h"
#include "tta/PawnLife.h"

#include <stdexcept>

using namespace tta;

TEST_CASE("hit freezes, then ragdolls, then respawns") {
    PawnLife life;
    CHECK(life.advance(1.0) == LifeEvent::None);
    REQUIRE(life.hit());
    CHECK(life.state() == LifeState::Frozen);
    CHECK(life.advance(0.11) == LifeEvent::None);
    CHECK(life.state() == LifeState::Frozen);
    CHECK(life.advance(0.02) == LifeEvent::StartRagdoll);
    CHECK(life.state() == LifeState::Ragdoll);
    CHECK(life.advance(kRagdollSeconds - 0.01) == LifeEvent::None);
    CHECK(life.advance(0.02) == LifeEvent::Respawn);
    CHECK(life.state() == LifeState::Alive);
}

TEST_CASE("hits are ignored until respawned") {
    PawnLife life;
    REQUIRE(life.hit());
    CHECK_FALSE(life.hit());
    life.advance(kImpactFreezeSeconds);
    CHECK_FALSE(life.hit());
    CHECK(life.state() == LifeState::Ragdoll);
}

TEST_CASE("one transition per advance") {
    PawnLife life;
    life.hit();
    CHECK(life.advance(10.0) == LifeEvent::StartRagdoll);
    CHECK(life.advance(10.0) == LifeEvent::Respawn);
}

TEST_CASE("negative dt rejected") {
    PawnLife life;
    CHECK_THROWS_AS(life.advance(-0.1), std::invalid_argument);
}

TEST_CASE("human form has no wings or halo") {
    CHECK(wingsVisible(Form::Angel));
    CHECK(haloVisible(Form::Angel));
    CHECK_FALSE(wingsVisible(Form::Human));
    CHECK_FALSE(haloVisible(Form::Human));
}

TEST_CASE("stun gets up in place after its time, no respawn") {
    PawnLife life;
    REQUIRE(life.stun(kStunSeconds));
    CHECK(life.state() == LifeState::Stunned);
    CHECK(life.advance(0.99) == LifeEvent::None);
    CHECK(life.state() == LifeState::Stunned);
    CHECK(life.advance(0.01) == LifeEvent::GetUp);
    CHECK(life.state() == LifeState::Alive);
}

TEST_CASE("stun only from alive, seconds must be positive") {
    PawnLife life;
    CHECK_THROWS_AS(life.stun(0.0), std::invalid_argument);
    CHECK_THROWS_AS(life.stun(-1.0), std::invalid_argument);
    REQUIRE(life.stun(kStunSeconds));
    CHECK_FALSE(life.stun(kStunSeconds));
    PawnLife trapped;
    REQUIRE(trapped.hit());
    CHECK_FALSE(trapped.stun(kStunSeconds));
    trapped.advance(kImpactFreezeSeconds);
    CHECK_FALSE(trapped.stun(kStunSeconds));
}

TEST_CASE("2 s punch immunity after getting up") {
    PawnLife life;
    REQUIRE(life.stun(kStunSeconds));
    CHECK_FALSE(life.punchImmune());
    REQUIRE(life.advance(kStunSeconds) == LifeEvent::GetUp);
    CHECK(life.punchImmune());
    CHECK_FALSE(life.stun(kStunSeconds));
    life.advance(1.99);
    CHECK(life.punchImmune());
    CHECK_FALSE(life.stun(kStunSeconds));
    life.advance(0.01);
    CHECK_FALSE(life.punchImmune());
    CHECK(life.stun(kStunSeconds));
}

TEST_CASE("immunity does not block trap hits") {
    PawnLife life;
    REQUIRE(life.stun(kStunSeconds));
    life.advance(kStunSeconds);
    REQUIRE(life.punchImmune());
    CHECK(life.hit());
    CHECK(life.state() == LifeState::Frozen);
}

TEST_CASE("trap hit during stun overrides it and respawns") {
    PawnLife life;
    REQUIRE(life.stun(kStunSeconds));
    life.advance(0.5);
    REQUIRE(life.hit());
    CHECK(life.state() == LifeState::Frozen);
    CHECK(life.advance(kImpactFreezeSeconds) == LifeEvent::StartRagdoll);
    CHECK(life.advance(kStunSeconds) == LifeEvent::None);  // stun time no longer applies
    CHECK(life.state() == LifeState::Ragdoll);
    CHECK(life.advance(kRagdollSeconds) == LifeEvent::Respawn);
    CHECK(life.state() == LifeState::Alive);
    CHECK_FALSE(life.punchImmune());
}
