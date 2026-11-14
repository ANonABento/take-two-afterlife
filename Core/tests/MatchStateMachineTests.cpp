#include "doctest/doctest.h"
#include "tta/MatchStateMachine.h"

#include <string>

using tta::MatchStateMachine;
using tta::Phase;

namespace doctest {
template <> struct StringMaker<Phase> {
    static String convert(Phase phase) { return tta::phaseName(phase); }
};
} // namespace doctest

namespace {
void playRound(MatchStateMachine& m) {
    REQUIRE(m.transition(Phase::Build));
    REQUIRE(m.transition(Phase::Run));
    REQUIRE(m.transition(Phase::Edit));
    REQUIRE(m.transition(Phase::Premiere));
}
} // namespace

TEST_CASE("full match runs the configured number of rounds") {
    MatchStateMachine m(tta::parseMatchSettings("{\"rounds\": 2}"));
    CHECK(m.phase() == Phase::Lobby);
    CHECK(m.round() == 0);
    playRound(m);
    CHECK(m.round() == 1);
    CHECK_FALSE(m.transition(Phase::Podium));
    playRound(m);
    CHECK(m.round() == 2);
    CHECK_FALSE(m.transition(Phase::Build));
    CHECK(m.transition(Phase::Podium));
    CHECK(m.transition(Phase::Lobby));
    CHECK(m.round() == 0);
}

TEST_CASE("illegal transitions are rejected and change nothing") {
    MatchStateMachine m(tta::MatchSettings{});
    CHECK_FALSE(m.transition(Phase::Run));
    CHECK_FALSE(m.transition(Phase::Premiere));
    CHECK_FALSE(m.transition(Phase::Lobby));
    CHECK(m.phase() == Phase::Lobby);
    REQUIRE(m.transition(Phase::Build));
    CHECK_FALSE(m.transition(Phase::Build));
    CHECK_FALSE(m.transition(Phase::Edit));
    CHECK_FALSE(m.transition(Phase::Lobby));
    CHECK(m.phase() == Phase::Build);
    CHECK(m.round() == 1);
}

TEST_CASE("phase timers come from settings") {
    MatchStateMachine m(tta::parseMatchSettings(
        "{\"buildSeconds\": 61, \"runSeconds\": 62, \"editSeconds\": 63}"));
    CHECK(m.phaseSeconds() == 0);
    m.transition(Phase::Build);
    CHECK(m.phaseSeconds() == 61);
    m.transition(Phase::Run);
    CHECK(m.phaseSeconds() == 62);
    m.transition(Phase::Edit);
    CHECK(m.phaseSeconds() == 63);
    m.transition(Phase::Premiere);
    CHECK(m.phaseSeconds() == 0);
}

TEST_CASE("phase names") {
    CHECK(std::string(tta::phaseName(Phase::Premiere)) == "Premiere");
}

TEST_CASE("custom settings change phase lengths and round count") {
    tta::MatchSettings s;
    s.rounds = 1;
    s.buildSeconds = 30;
    s.runSeconds = 45;
    s.editSeconds = 20;
    MatchStateMachine m{s};
    REQUIRE(m.transition(Phase::Build));
    CHECK(m.phaseSeconds() == 30);
    REQUIRE(m.transition(Phase::Run));
    CHECK(m.phaseSeconds() == 45);
    REQUIRE(m.transition(Phase::Edit));
    CHECK(m.phaseSeconds() == 20);
    REQUIRE(m.transition(Phase::Premiere));
    CHECK_FALSE(m.transition(Phase::Build));
    CHECK(m.transition(Phase::Podium));
}
