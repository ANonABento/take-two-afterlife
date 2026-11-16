import json
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))
import build_ui_chrome as chrome  # noqa: E402

ART = ROOT / "Art" / "UI"
CPP = (ROOT / "Source" / "TakeTwoAfterlife" / "Private" / "TTAUIStyle.cpp").read_text(encoding="utf-8-sig")
BUTTONS = ["T_UI_Button", "T_UI_ButtonHover", "T_UI_ButtonPressed", "T_UI_ButtonPrimary"]


class UIChromeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.out = Path(cls.tmp.name)
        cls.doc = chrome.build(cls.out)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_button_states_share_one_size(self):
        sizes = {Image.open(ART / f"{b}.png").size for b in BUTTONS}
        self.assertEqual(sizes, {chrome.BUTTON_SIZE})

    def test_slice_margins_are_radius_plus_outline(self):
        assets = json.loads((ART / chrome.SLICES_NAME).read_text(encoding="utf-8"))["assets"]
        for name, (_, radius, _, sliced) in chrome.CHROME.items():
            m = assets[name]
            edge = radius + chrome.OUTLINE if sliced else 0
            shadow = chrome.SHADOW if sliced else 0
            self.assertEqual((m["left"], m["top"], m["right"], m["bottom"]), (edge, edge, edge, edge + shadow), name)

    def test_drawn_corners_fit_inside_the_margin(self):
        for name, (_, radius, _, _) in chrome.CHROME.items():
            self.assertLessEqual(chrome.corner_extent(Image.open(ART / f"{name}.png")), radius, name)

    def test_committed_art_matches_a_fresh_build(self):
        committed = json.loads((ART / chrome.SLICES_NAME).read_text(encoding="utf-8"))
        self.assertEqual(committed, self.doc)
        for name in chrome.CHROME:
            fresh = Image.open(self.out / f"{name}.png")
            self.assertEqual(Image.open(ART / f"{name}.png").tobytes(), fresh.tobytes(), name)

    def test_outline_is_ink_and_canvas_has_alpha(self):
        ink = chrome.palette()["Ink"]
        for name, ((w, h), _, _, _) in chrome.CHROME.items():
            img = Image.open(ART / f"{name}.png").convert("RGBA")
            self.assertEqual(img.getpixel((w // 2, 2)), ink, name)
            self.assertEqual(img.getpixel((0, 0))[3], 0, name)

    def test_style_reads_measured_slices_not_typed_values(self):
        self.assertIn(chrome.SLICES_NAME, CPP)
        self.assertNotRegex(CPP, r"\{ETTAAsset::\w+, ETTAColor::\w+, [\d.]+f")


if __name__ == "__main__":
    unittest.main()
