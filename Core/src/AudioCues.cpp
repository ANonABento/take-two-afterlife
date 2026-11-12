#include "tta/AudioCues.h"

#include <stdexcept>

namespace tta {

const char* audioEventName(AudioEvent event) {
    switch (event) {
        case AudioEvent::PhaseLobby: return "PhaseLobby";
        case AudioEvent::PhaseBuild: return "PhaseBuild";
        case AudioEvent::PhaseRun: return "PhaseRun";
        case AudioEvent::PhaseEdit: return "PhaseEdit";
        case AudioEvent::PhasePremiere: return "PhasePremiere";
        case AudioEvent::PhasePodium: return "PhasePodium";
        case AudioEvent::TrapPlaced: return "TrapPlaced";
        case AudioEvent::RunCountdown: return "RunCountdown";
        case AudioEvent::RunHurry: return "RunHurry";
        case AudioEvent::Wipeout: return "Wipeout";
        case AudioEvent::Smite: return "Smite";
        case AudioEvent::GateReached: return "GateReached";
        case AudioEvent::PremiereIntro: return "PremiereIntro";
        case AudioEvent::RatingOpen: return "RatingOpen";
        case AudioEvent::RatingReveal: return "RatingReveal";
        case AudioEvent::Count: break;
    }
    throw std::invalid_argument("audioEventName: not an event");
}

const std::vector<AudioCue>& audioCues() {
    static const std::vector<AudioCue> table = {
        {"VO_Lobby_01", AudioKind::Narrator, AudioEvent::PhaseLobby, "Welcome to the afterlife. Grab a halo and get comfy."},
        {"VO_Lobby_02", AudioKind::Narrator, AudioEvent::PhaseLobby, "Another batch of fresh souls. Lovely. Pick a hat."},
        {"VO_Lobby_03", AudioKind::Narrator, AudioEvent::PhaseLobby, "The couch is reserved for directors. That's all of you."},
        {"VO_Lobby_04", AudioKind::Narrator, AudioEvent::PhaseLobby, "Waiting on the host. Heaven is patient. I am not."},
        {"VO_Build_01", AudioKind::Narrator, AudioEvent::PhaseBuild, "Build time! Place your traps. Be creative. Be cruel."},
        {"VO_Build_02", AudioKind::Narrator, AudioEvent::PhaseBuild, "Remember, cameras are free. Regret is also free."},
        {"VO_Build_03", AudioKind::Narrator, AudioEvent::PhaseBuild, "Hide a camera where the screaming will be."},
        {"VO_Build_04", AudioKind::Narrator, AudioEvent::PhaseBuild, "One hundred points of mischief. Spend it wisely."},
        {"VO_Build_05", AudioKind::Narrator, AudioEvent::PhaseBuild, "A trap nobody can beat scores nothing. Be fair. Ish."},
        {"VO_Build_06", AudioKind::Narrator, AudioEvent::PhaseBuild, "Your traps stay secret until the run. Shhh."},
        {"VO_Countdown_01", AudioKind::Narrator, AudioEvent::RunCountdown, "Three. Two. One. Live!"},
        {"VO_Countdown_02", AudioKind::Narrator, AudioEvent::RunCountdown, "Places, mortals. And... action!"},
        {"VO_Countdown_03", AudioKind::Narrator, AudioEvent::RunCountdown, "Cameras rolling. Try not to die. Too much."},
        {"VO_Run_01", AudioKind::Narrator, AudioEvent::PhaseRun, "You are mortal again. Enjoy it while it lasts."},
        {"VO_Run_02", AudioKind::Narrator, AudioEvent::PhaseRun, "Run for the gate! Mind the bananas."},
        {"VO_Run_03", AudioKind::Narrator, AudioEvent::PhaseRun, "Don't forget your handheld. Film everything."},
        {"VO_Run_04", AudioKind::Narrator, AudioEvent::PhaseRun, "Die, and you start over. Such is life."},
        {"VO_Run_05", AudioKind::Narrator, AudioEvent::PhaseRun, "Go go go! Somebody film that!"},
        {"VO_Hurry_01", AudioKind::Narrator, AudioEvent::RunHurry, "Ten seconds left. The sky is getting grumpy."},
        {"VO_Hurry_02", AudioKind::Narrator, AudioEvent::RunHurry, "Hurry up, or the lightning will help you."},
        {"VO_Hurry_03", AudioKind::Narrator, AudioEvent::RunHurry, "Clock's almost out. I'd run faster, personally."},
        {"VO_Hurry_04", AudioKind::Narrator, AudioEvent::RunHurry, "Final seconds! Heaven Cam is warming up."},
        {"VO_Wipeout_01", AudioKind::Narrator, AudioEvent::Wipeout, "Ooh. That's going in the trailer."},
        {"VO_Wipeout_02", AudioKind::Narrator, AudioEvent::Wipeout, "Gravity: one. You: zero."},
        {"VO_Wipeout_03", AudioKind::Narrator, AudioEvent::Wipeout, "Somebody got that on camera, right?"},
        {"VO_Wipeout_04", AudioKind::Narrator, AudioEvent::Wipeout, "That looked like it hurt. Good thing you're already dead."},
        {"VO_Wipeout_05", AudioKind::Narrator, AudioEvent::Wipeout, "Directors, please mark that moment."},
        {"VO_Wipeout_06", AudioKind::Narrator, AudioEvent::Wipeout, "Beautiful. Do it again."},
        {"VO_Wipeout_07", AudioKind::Narrator, AudioEvent::Wipeout, "And down they go."},
        {"VO_Wipeout_08", AudioKind::Narrator, AudioEvent::Wipeout, "Classic."},
        {"VO_Smite_01", AudioKind::Narrator, AudioEvent::Smite, "Time's up. Smite incoming!"},
        {"VO_Smite_02", AudioKind::Narrator, AudioEvent::Smite, "Lightning delivery for one slowpoke."},
        {"VO_Smite_03", AudioKind::Narrator, AudioEvent::Smite, "Heaven Cam caught that. Of course it did."},
        {"VO_Smite_04", AudioKind::Narrator, AudioEvent::Smite, "Zap. That's showbiz."},
        {"VO_Smite_05", AudioKind::Narrator, AudioEvent::Smite, "Out of time, out of luck."},
        {"VO_Gate_01", AudioKind::Narrator, AudioEvent::GateReached, "Through the gate! Look at you, alive and everything."},
        {"VO_Gate_02", AudioKind::Narrator, AudioEvent::GateReached, "Made it! Your halo awaits."},
        {"VO_Gate_03", AudioKind::Narrator, AudioEvent::GateReached, "A finisher! Somebody's traps need work."},
        {"VO_Edit_01", AudioKind::Narrator, AudioEvent::PhaseEdit, "Edit time. Ninety seconds to make art."},
        {"VO_Edit_02", AudioKind::Narrator, AudioEvent::PhaseEdit, "Trim the boring bits. Keep the screaming."},
        {"VO_Edit_03", AudioKind::Narrator, AudioEvent::PhaseEdit, "Slow-mo makes everything funnier. Trust me."},
        {"VO_Edit_04", AudioKind::Narrator, AudioEvent::PhaseEdit, "Out of time? I'll auto-edit it for you. Badly."},
        {"VO_Edit_05", AudioKind::Narrator, AudioEvent::PhaseEdit, "Find your best angle. Not theirs. Yours."},
        {"VO_Premiere_01", AudioKind::Narrator, AudioEvent::PhasePremiere, "Welcome to the premiere. Find a cloud and sit down."},
        {"VO_Premiere_02", AudioKind::Narrator, AudioEvent::PhasePremiere, "Lights down. Phones off. Halos dimmed."},
        {"VO_Premiere_03", AudioKind::Narrator, AudioEvent::PhasePremiere, "Popcorn is complimentary. It is also imaginary."},
        {"VO_Intro_01", AudioKind::Narrator, AudioEvent::PremiereIntro, "Our next cut, from a visionary director."},
        {"VO_Intro_02", AudioKind::Narrator, AudioEvent::PremiereIntro, "Next up: a tragedy in thirty seconds."},
        {"VO_Intro_03", AudioKind::Narrator, AudioEvent::PremiereIntro, "Presenting a film about poor life choices."},
        {"VO_Intro_04", AudioKind::Narrator, AudioEvent::PremiereIntro, "This next one contains scenes of mild bonking."},
        {"VO_Intro_05", AudioKind::Narrator, AudioEvent::PremiereIntro, "Roll the tape!"},
        {"VO_Rating_01", AudioKind::Narrator, AudioEvent::RatingOpen, "Rate it! How funny? How evil?"},
        {"VO_Rating_02", AudioKind::Narrator, AudioEvent::RatingOpen, "Ten seconds. Be honest. Be brutal."},
        {"VO_Rating_03", AudioKind::Narrator, AudioEvent::RatingOpen, "Scores, please. No rating your own work."},
        {"VO_Reveal_01", AudioKind::Narrator, AudioEvent::RatingReveal, "The critics have spoken."},
        {"VO_Reveal_02", AudioKind::Narrator, AudioEvent::RatingReveal, "Ooh, the audience loved that one."},
        {"VO_Reveal_03", AudioKind::Narrator, AudioEvent::RatingReveal, "Well. Somebody liked it."},
        {"VO_Podium_01", AudioKind::Narrator, AudioEvent::PhasePodium, "And the winner of the afterlife is..."},
        {"VO_Podium_02", AudioKind::Narrator, AudioEvent::PhasePodium, "Take a bow, directors. You've earned your wings."},
        {"VO_Podium_03", AudioKind::Narrator, AudioEvent::PhasePodium, "That's a wrap! Thanks for dying with us."},
        {"MUS_Lobby", AudioKind::Music, AudioEvent::PhaseLobby, ""},
        {"MUS_Build", AudioKind::Music, AudioEvent::PhaseBuild, ""},
        {"MUS_Run", AudioKind::Music, AudioEvent::PhaseRun, ""},
        {"MUS_Edit", AudioKind::Music, AudioEvent::PhaseEdit, ""},
        {"MUS_Premiere", AudioKind::Music, AudioEvent::PhasePremiere, ""},
        {"MUS_Podium", AudioKind::Music, AudioEvent::PhasePodium, ""},
        {"SFX_TrapPlace", AudioKind::Sfx, AudioEvent::TrapPlaced, ""},
        {"SFX_CountdownBeep", AudioKind::Sfx, AudioEvent::RunCountdown, ""},
        {"SFX_ClockTick", AudioKind::Sfx, AudioEvent::RunHurry, ""},
        {"SFX_ImpactBonk", AudioKind::Sfx, AudioEvent::Wipeout, ""},
        {"SFX_Thunder", AudioKind::Sfx, AudioEvent::Smite, ""},
        {"SFX_GateChime", AudioKind::Sfx, AudioEvent::GateReached, ""},
        {"SFX_ProjectorStart", AudioKind::Sfx, AudioEvent::PremiereIntro, ""},
        {"SFX_RatingDing", AudioKind::Sfx, AudioEvent::RatingOpen, ""},
        {"SFX_Applause", AudioKind::Sfx, AudioEvent::RatingReveal, ""},
        {"SFX_Fanfare", AudioKind::Sfx, AudioEvent::PhasePodium, ""},
    };
    return table;
}

std::vector<const AudioCue*> cuesFor(AudioEvent event, AudioKind kind) {
    std::vector<const AudioCue*> out;
    for (const AudioCue& cue : audioCues()) {
        if (cue.event == event && cue.kind == kind) out.push_back(&cue);
    }
    return out;
}

AudioEvent phaseEvent(Phase phase) {
    return static_cast<AudioEvent>(static_cast<int>(phase));
}

const AudioCue& musicFor(Phase phase) {
    const auto music = cuesFor(phaseEvent(phase), AudioKind::Music);
    if (music.size() != 1) throw std::logic_error(std::string("musicFor: need one track for ") + phaseName(phase));
    return *music.front();
}

const AudioCue* narratorLine(AudioEvent event, int n) {
    const auto lines = cuesFor(event, AudioKind::Narrator);
    if (lines.empty()) return nullptr;
    const int size = static_cast<int>(lines.size());
    return lines[static_cast<size_t>(((n % size) + size) % size)];
}

std::string assetPath(const AudioCue& cue) {
    const char* folder = cue.kind == AudioKind::Narrator ? "VO" : cue.kind == AudioKind::Music ? "Music" : "SFX";
    return std::string("/Game/Audio/") + folder + "/" + cue.id + "." + cue.id;
}

}  // namespace tta
