#pragma once

#include "tta/MatchStateMachine.h"

#include <set>
#include <string>
#include <vector>

namespace tta {

// Everything the game says or plays: narrator lines (ElevenLabs, baked to files), one
// music track per phase (WeLoveIndies) and SFX. This table is the single source of truth;
// UE asset paths and the TTS script are derived from it.
enum class AudioKind { Narrator, Music, Sfx };

// The first six mirror tta::Phase in order (phaseEvent relies on it).
enum class AudioEvent {
    PhaseLobby, PhaseBuild, PhaseRun, PhaseEdit, PhasePremiere, PhasePodium,
    TrapPlaced, RunCountdown, RunHurry, Wipeout, Smite, GateReached,
    PremiereIntro, RatingOpen, RatingReveal,
    Count
};

struct AudioCue {
    const char* id;    // asset name, e.g. "VO_Build_01"
    AudioKind kind;
    AudioEvent event;
    const char* text;  // narrator script; empty for music/SFX
};

const char* audioEventName(AudioEvent event);
const std::vector<AudioCue>& audioCues();
std::vector<const AudioCue*> cuesFor(AudioEvent event, AudioKind kind);
// The event fired when a phase starts.
AudioEvent phaseEvent(Phase phase);
// The music cue for a phase.
const AudioCue& musicFor(Phase phase);
// Deterministic pick among an event's narrator lines: the n-th call gets line n mod count.
// Returns nullptr when the event has no narrator line.
const AudioCue* narratorLine(AudioEvent event, int n);
// UE object path, e.g. "/Game/Audio/VO/VO_Build_01.VO_Build_01".
std::string assetPath(const AudioCue& cue);

// Remembers asset paths that failed to load this session, so a missing sound is reported
// once (reportMissing returns true only the first time) and then skipped without a lookup.
class MissingAssetLog {
public:
    bool reportMissing(const std::string& path) { return missing_.insert(path).second; }
    bool isKnownMissing(const std::string& path) const { return missing_.count(path) > 0; }
    std::size_t count() const { return missing_.size(); }
    void reset() { missing_.clear(); }

private:
    std::set<std::string> missing_;
};

}  // namespace tta
