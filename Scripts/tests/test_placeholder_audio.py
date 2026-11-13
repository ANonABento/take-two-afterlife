import re, sys, tempfile, unittest, wave
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
ROOT = SCRIPTS.parent
sys.path.insert(0, str(SCRIPTS))
import placeholder_audio as P


class PlaceholderAudioTests(unittest.TestCase):
    def test_every_core_cue_gets_a_path(self):
        cues = P.cues()
        ids = [c[0] for c in cues]
        src = (ROOT / "Core" / "src" / "AudioCues.cpp").read_text()
        self.assertEqual(len(ids), len(re.findall(r'\{"(?:VO|MUS|SFX)_\w+", AudioKind::', src)))
        self.assertEqual(len(ids), len(set(ids)))
        paths = {c[0]: P.asset_path(*c) for c in cues}
        # same convention as tta::assetPath (Core/tests/AudioCuesTests.cpp)
        self.assertEqual(paths["MUS_Lobby"], "/Game/Audio/Music/MUS_Lobby.MUS_Lobby")
        self.assertEqual(paths["VO_Lobby_01"], "/Game/Audio/VO/VO_Lobby_01.VO_Lobby_01")
        self.assertEqual(paths["SFX_Thunder"], "/Game/Audio/SFX/SFX_Thunder.SFX_Thunder")

    def test_real_files_are_kept(self):
        with tempfile.TemporaryDirectory() as d:
            content = Path(d)
            (content / "Audio" / "Music").mkdir(parents=True)
            (content / "Audio" / "Music" / "MUS_Lobby.uasset").write_bytes(b"x")
            todo = P.missing(P.cues(), content)
            names = [c[0] for c in todo]
            self.assertNotIn("MUS_Lobby", names)
            self.assertIn("VO_Lobby_01", names)
            self.assertEqual(len(todo), len(P.cues()) - 1)

    def test_silent_wav(self):
        with tempfile.TemporaryDirectory() as d:
            f = P.write_silent_wav(Path(d) / "s.wav", seconds=0.1)
            with wave.open(str(f)) as w:
                self.assertEqual(w.getnchannels(), 1)
                frames = w.readframes(w.getnframes())
            self.assertEqual(len(frames), int(P.RATE * 0.1) * 2)
            self.assertEqual(set(frames), {0})


if __name__ == "__main__":
    unittest.main()
