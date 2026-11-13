#include "doctest/doctest.h"
#include "tta/AudioCues.h"

#include <set>
#include <string>

using namespace tta;

TEST_CASE("phase events line up with phases") {
    CHECK(phaseEvent(Phase::Lobby) == AudioEvent::PhaseLobby);
    CHECK(phaseEvent(Phase::Podium) == AudioEvent::PhasePodium);
    CHECK(std::string(audioEventName(phaseEvent(Phase::Premiere))) == "PhasePremiere");
}

TEST_CASE("every event has a cue; every phase has one music track and a line") {
    for (int e = 0; e < static_cast<int>(AudioEvent::Count); ++e) {
        const auto event = static_cast<AudioEvent>(e);
        CAPTURE(audioEventName(event));
        CHECK(cuesFor(event, AudioKind::Narrator).size() + cuesFor(event, AudioKind::Sfx).size() +
                  cuesFor(event, AudioKind::Music).size() > 0);
    }
    for (Phase p : {Phase::Lobby, Phase::Build, Phase::Run, Phase::Edit, Phase::Premiere, Phase::Podium}) {
        CAPTURE(phaseName(p));
        CHECK(std::string(musicFor(p).id) == std::string("MUS_") + phaseName(p));
        CHECK(narratorLine(phaseEvent(p), 0) != nullptr);
    }
}

TEST_CASE("60 narrator lines, unique ids, text only on lines") {
    std::set<std::string> ids;
    int narrator = 0;
    for (const AudioCue& cue : audioCues()) {
        CHECK(ids.insert(cue.id).second);
        const bool hasText = !std::string(cue.text).empty();
        CHECK(hasText == (cue.kind == AudioKind::Narrator));
        if (cue.kind == AudioKind::Narrator) ++narrator;
    }
    CHECK(narrator == 60);
}

TEST_CASE("narrator picks round-robin; asset paths follow the folder layout") {
    const auto lines = cuesFor(AudioEvent::Wipeout, AudioKind::Narrator);
    REQUIRE(lines.size() > 1);
    CHECK(narratorLine(AudioEvent::Wipeout, 0) == lines[0]);
    CHECK(narratorLine(AudioEvent::Wipeout, static_cast<int>(lines.size())) == lines[0]);
    CHECK(narratorLine(AudioEvent::Wipeout, 1) == lines[1]);
    CHECK(narratorLine(AudioEvent::TrapPlaced, 0) == nullptr);
    CHECK(assetPath(*lines[0]) == "/Game/Audio/VO/VO_Wipeout_01.VO_Wipeout_01");
    CHECK(assetPath(musicFor(Phase::Run)) == "/Game/Audio/Music/MUS_Run.MUS_Run");
}

TEST_CASE("a missing asset is reported once per session, then skipped quietly") {
    MissingAssetLog log;
    const std::string lobby = assetPath(musicFor(Phase::Lobby));
    CHECK_FALSE(log.isKnownMissing(lobby));
    CHECK(log.reportMissing(lobby));        // first play: warn
    CHECK(log.isKnownMissing(lobby));
    for (int i = 0; i < 10; ++i) CHECK_FALSE(log.reportMissing(lobby));  // later plays: silent
    CHECK(log.reportMissing("/Game/Audio/VO/VO_Lobby_01.VO_Lobby_01"));  // other assets still warn once
    CHECK(log.count() == 2);
    log.reset();                            // new session
    CHECK(log.reportMissing(lobby));
}
