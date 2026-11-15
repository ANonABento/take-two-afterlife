import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))
import toon_spec  # noqa: E402

SRC = ROOT / "Source" / "TakeTwoAfterlife"
WIDGETS = ("TTAMenuWidget", "TTABuildHUD", "TTAResultsWidget", "TTAPodiumWidget")
PALETTE_HEX = {h.upper() for _, h in toon_spec.PALETTE}


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def ui_sources():
    for name in WIDGETS + ("TTAUIStyle",):
        yield name, strip_comments((SRC / "Private" / f"{name}.cpp").read_text() + (SRC / "Public" / f"{name}.h").read_text())


class UIStyleTests(unittest.TestCase):
    def test_every_widget_uses_shared_style(self):
        for name in WIDGETS:
            cpp = (SRC / "Private" / f"{name}.cpp").read_text()
            self.assertIn('#include "TTAUIStyle.h"', cpp, name)
            self.assertIn("UTTAUIStyle::", cpp, name)
            self.assertRegex(cpp, r"SafeArea", f"{name} ignores the safe area")

    def test_no_colour_outside_palette(self):
        named = re.compile(r"\bF(?:Linear)?Color::(?!FromHex|LerpUsingHSV)[A-Z]\w*")
        literal = re.compile(r"\bF(?:Linear)?Color\s*\(\s*[-\d.]")
        for name, code in ui_sources():
            self.assertIsNone(named.search(code), f"{name}: named engine colour")
            self.assertIsNone(literal.search(code), f"{name}: literal colour")
            # A whole string literal that is a hex colour (has a digit, so "ABCDEF" alphabets pass).
            for lit in re.findall(r'"#?([0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?)"', code):
                if re.search(r"\d", lit):
                    self.assertIn(lit.upper()[:6], PALETTE_HEX, f"{name}: hex {lit}")
            for call in re.findall(r"FromHex\(([^;]*)\)", code):
                self.assertIn("kPaletteHex", call, f"{name}: FromHex outside the palette")
            if name != "TTAUIStyle":
                self.assertNotIn("FromHex", code, f"{name}: colours must go through UTTAUIStyle::Colour")

    def test_style_sheet_matches_palette(self):
        doc = (ROOT / "docs" / "art" / "palette.md").read_text()
        found = re.findall(r"\|\s*\d+\s*\|\s*(\w+)\s*\|\s*#([0-9A-F]{6})\s*\|", doc)
        self.assertEqual(found, [(n, h) for n, h in toon_spec.PALETTE])
        for font in ("Lilita One", "Nunito"):
            self.assertIn(font, doc)

    def test_config_points_at_imported_fonts_and_safe_area(self):
        ini = (ROOT / "Config" / "DefaultGame.ini").read_text()
        section = ini.split("[/Script/TakeTwoAfterlife.TTAUIStyle]", 1)[1].split("\n[", 1)[0]
        importer = (ROOT / "Scripts" / "import_fonts.py").read_text()
        for key, asset in (("DisplayFont", "LilitaOne"), ("BodyFont", "Nunito")):
            self.assertIn(f"{key}=/Game/UI/Fonts/{asset}.{asset}", section)
            self.assertIn(f'"{asset}"', importer)
        self.assertIn("SafeArea=(Left=96,Top=54,Right=96,Bottom=54)", section)
        for screen in ("Menu", "HUD", "Results", "Podium"):
            self.assertRegex(section, rf"\n{screen}Background=")

    def test_hud_shows_phase_timer_budget(self):
        hud = (SRC / "Private" / "TTABuildHUD.cpp").read_text()
        for needle in ("GetPhase()", "GetSecondsLeft()", "Own.remaining(Me)", "kTimer", "UTTAResultsWidget", "UTTAPodiumWidget"):
            self.assertIn(needle, hud)


if __name__ == "__main__":
    unittest.main()
