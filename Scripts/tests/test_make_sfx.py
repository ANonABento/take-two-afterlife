import subprocess, sys, tempfile, unittest, wave
from pathlib import Path

import numpy as np

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))
import make_sfx as S


class MakeSfxTests(unittest.TestCase):
    def test_script_writes_all_ten_valid_wavs(self):
        with tempfile.TemporaryDirectory() as d:
            subprocess.run([sys.executable, str(SCRIPTS / "make_sfx.py"), d], check=True, capture_output=True)
            self.assertEqual(len(S.SOUNDS), 10)
            self.assertEqual(sorted(p.stem for p in Path(d).glob("*.wav")), sorted(S.SOUNDS))
            for name, (_, seconds) in S.SOUNDS.items():
                with self.subTest(name=name), wave.open(str(Path(d) / f"{name}.wav")) as w:
                    self.assertEqual((w.getnchannels(), w.getsampwidth(), w.getframerate()), (1, 2, 44100))
                    self.assertAlmostEqual(w.getnframes() / 44100, seconds, delta=seconds * 0.1)
                    x = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(float)
                    peak_db = 20 * np.log10(np.max(np.abs(x)) / 32767)
                    self.assertLessEqual(peak_db, 0.0)
                    self.assertAlmostEqual(peak_db, -1.0, delta=0.05)
                    self.assertLess(abs(x[0]), 0.05 * 32767)
                    self.assertLess(abs(x[-1]), 0.05 * 32767)


if __name__ == "__main__":
    unittest.main()
