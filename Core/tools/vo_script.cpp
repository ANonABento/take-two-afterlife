// Prints the narrator script as CSV (id,text) for batch TTS in ElevenLabs.
#include "tta/AudioCues.h"

#include <iostream>

int main() {
    std::cout << "id,text\n";
    for (const tta::AudioCue& cue : tta::audioCues()) {
        if (cue.kind == tta::AudioKind::Narrator) std::cout << cue.id << ",\"" << cue.text << "\"\n";
    }
    return 0;
}
