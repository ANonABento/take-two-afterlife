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
