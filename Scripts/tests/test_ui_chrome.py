"""The menu chrome is cut from the contact sheet, not drawn as flat rectangles."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))
import cut_ui_sheet as sheet  # noqa: E402

ART = ROOT / "Art" / "UI"
CPP = (ROOT / "Source" / "TakeTwoAfterlife" / "Private" / "TTAUIStyle.cpp").read_text(encoding="utf-8-sig")


class UIChromeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.out = Path(cls.tmp.name)
        cls.doc = sheet.build(cls.out)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_committed_art_matches_a_fresh_cut(self):
        committed = json.loads((ART / sheet.SLICES_NAME).read_text(encoding="utf-8"))
        self.assertEqual(committed, self.doc)
        for name in sheet.BOXES:
            fresh = Image.open(self.out / f"{name}.png")
            self.assertEqual(Image.open(ART / f"{name}.png").tobytes(), fresh.tobytes(), name)

    def test_slice_margins_leave_a_stretchable_centre(self):
        for name, entry in self.doc["assets"].items():
            if name not in sheet.SLICED:
                self.assertEqual((entry["left"], entry["top"], entry["right"], entry["bottom"]), (0, 0, 0, 0), name)
                continue
            self.assertLess(entry["left"] + entry["right"], entry["width"], name)
            self.assertLess(entry["top"] + entry["bottom"], entry["height"], name)
            self.assertGreater(entry["left"], 0, name)
            self.assertGreater(entry["top"], 0, name)

    def test_corners_are_transparent(self):
        for name in sheet.SLICED:
            img = Image.open(ART / f"{name}.png")
            self.assertEqual(img.getpixel((0, 0))[3], 0, name)

    def test_style_reads_measured_slices_not_typed_values(self):
        self.assertIn(sheet.SLICES_NAME, CPP)
        self.assertNotRegex(CPP, r"\{ETTAAsset::\w+, ETTAColor::\w+, [\d.]+f")

    def test_title_backdrop_is_not_the_sheet_crop(self):
        # The sheet's backdrop thumbnail is a few hundred pixels. The game keeps the large painting.
        self.assertGreater(Image.open(ART / "T_UI_TitleBackdrop.png").size[0], 1000)


if __name__ == "__main__":
    unittest.main()
