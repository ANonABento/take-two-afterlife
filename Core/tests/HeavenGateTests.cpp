#include "doctest/doctest.h"
#include "tta/HeavenGate.h"

using namespace tta;

TEST_CASE("heaven gate counts each ready player once, only while open") {
    HeavenGate gate;
    gate.setPlayers(4);
    CHECK_FALSE(gate.markReady(1));  // closed until the host presses START
    CHECK(gate.readyCount() == 0);
    gate.open();
    CHECK(gate.markReady(1));
    CHECK_FALSE(gate.markReady(1));  // jumping through twice
    CHECK(gate.markReady(2));
    CHECK(gate.readyCount() == 2);
    CHECK(gate.players() == 4);
    gate.setPlayers(-3);
    CHECK(gate.players() == 0);
}

TEST_CASE("heaven gate travels when everyone is ready") {
    HeavenGate gate;
    gate.setPlayers(2);
    CHECK_FALSE(gate.shouldTravel());
    gate.open();
    CHECK_FALSE(gate.shouldTravel());  // open but nobody through yet
    gate.markReady(10);
    CHECK_FALSE(gate.shouldTravel());
    gate.markReady(11);
    CHECK(gate.shouldTravel());
}

TEST_CASE("heaven gate travels 20 s after the first ready player") {
    HeavenGate gate;
    gate.setPlayers(4);
    gate.open();
    gate.tick(100.0);  // clock waits for the first jump
    CHECK_FALSE(gate.shouldTravel());
    gate.markReady(1);
    gate.tick(HeavenGate::kTimeoutSeconds - 0.5);
    CHECK_FALSE(gate.shouldTravel());
    gate.tick(0.5);
    CHECK(gate.shouldTravel());
}
