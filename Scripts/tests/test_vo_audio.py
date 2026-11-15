"""Narrator WAVs (ElevenLabs) match the VO cue ids in Core/src/AudioCues.cpp."""
import re
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VO_DIR = ROOT / "Content" / "Audio" / "VO"


def vo_ids():
    src = (ROOT / "Core" / "src" / "AudioCues.cpp").read_text(encoding="utf-8")
    return sorted(set(re.findall(r'"(VO_\w+)"', src)))


class VoAudioTest(unittest.TestCase):
    def test_sixty_cues(self):
        self.assertEqual(len(vo_ids()), 60)

    def test_wavs_match_cues(self):
        wavs = sorted(p.stem for p in VO_DIR.glob("VO_*.wav"))
        if not wavs:
            self.skipTest("VO WAVs not unpacked yet (VO_wavs.zip)")
        self.assertEqual(wavs, vo_ids())
        for name in wavs:
            with wave.open(str(VO_DIR / f"{name}.wav")) as w:
                self.assertEqual(w.getframerate(), 44100)
                self.assertGreater(w.getnframes(), 0)
