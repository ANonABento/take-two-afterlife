"""Real music + SFX WAVs in Content/Audio (built by Scripts/build_audio.py) and their CREDITS rows."""
import sys
import unittest
import wave
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "Scripts"))
import build_audio as B  # noqa: E402
import placeholder_audio as P  # noqa: E402

MAX_BYTES = 3 * 1024 * 1024
FILES = [B.MUSIC_DIR / f"{c}.wav" for c in B.MUSIC] + [B.SFX_DIR / f"{c}.wav" for c in B.SFX.values()]
BUILT = any(f.exists() for f in FILES)


class AudioManifest(unittest.TestCase):
    def test_covers_every_music_and_sfx_cue_the_game_loads(self):
        cues = {cid for cid, folder in P.cues() if folder in ("Music", "SFX")}
        wanted = set(B.MUSIC) | set(B.SFX.values())
        self.assertTrue(wanted <= cues, wanted - cues)
        self.assertEqual(len(wanted), 16)

    def test_every_cue_has_a_credits_row(self):
        credits = (REPO / "CREDITS.md").read_text(encoding="utf-8")
        audio = credits.split("## Audio", 1)[1].split("\n## ", 1)[0]
        rows = [line for line in audio.splitlines() if line.startswith("|")]
        for cue in list(B.MUSIC) + list(B.SFX.values()):
            self.assertTrue(any(f"`{cue}`" in r for r in rows), cue)
        for title, author, page, _ in B.MUSIC.values():
            self.assertTrue(any(title in r and author in r and page in r and "CC0" in r for r in rows), title)


@unittest.skipUnless(BUILT, "audio not built yet: run python Scripts/build_audio.py")
class AudioFiles(unittest.TestCase):
    def test_all_files_exist_decode_and_are_small(self):
        for f in FILES:
            with self.subTest(f.name):
                self.assertTrue(f.exists(), f"{f} missing: run python Scripts/build_audio.py")
                self.assertLess(f.stat().st_size, MAX_BYTES)
                with wave.open(str(f)) as w:
                    self.assertEqual(w.getframerate(), B.RATE)
                    self.assertEqual(w.getsampwidth(), 2)
                    self.assertGreater(w.getnframes(), B.RATE // 20)
                    self.assertTrue(any(w.readframes(w.getnframes())))


if __name__ == "__main__":
    unittest.main()
