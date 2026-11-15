import sys
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))
import normalize_ui_icons as icons  # noqa: E402

TOLERANCE = 4


class UIIconTests(unittest.TestCase):
    def icon_paths(self):
        paths = sorted(icons.ART.glob(icons.PATTERN))
        self.assertTrue(paths, "no icons found")
        return paths

    def test_icons_are_square_canvas(self):
        for p in self.icon_paths():
            self.assertEqual(Image.open(p).size, (icons.CANVAS, icons.CANVAS), p.name)

    def test_icons_padded_and_centred(self):
        for p in self.icon_paths():
            left, top, right, bottom = icons.content_box(Image.open(p))
            pads = (left, top, icons.CANVAS - right, icons.CANVAS - bottom)
            # Long axis fills the inner box: its edges sit at the padding.
            self.assertLessEqual(abs(min(pads) - icons.PADDING), TOLERANCE, (p.name, pads))
            # No edge is tighter than the padding, and the glyph is centred.
            self.assertGreaterEqual(min(pads), icons.PADDING - TOLERANCE, (p.name, pads))
            self.assertLessEqual(abs(pads[0] - pads[2]), 2, (p.name, pads))
            self.assertLessEqual(abs(pads[1] - pads[3]), 2, (p.name, pads))

    def test_normalize_is_stable(self):
        img = Image.open(self.icon_paths()[0])
        again = icons.normalize(img)
        self.assertEqual(icons.content_box(again), icons.content_box(img))


if __name__ == "__main__":
    unittest.main()
