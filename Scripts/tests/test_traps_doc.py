"""Trap design doc: 7 trap sections, no empty fields, settings and costs match Core/src/Placement.cpp."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOC = ROOT / "docs" / "design" / "traps.md"
SOURCE = ROOT / "Core" / "src" / "Placement.cpp"
FIELDS = ("Footprint", "Setting 1", "Setting 2", "Cost", "Hit", "Maps", "Code type")
CODE_TYPES = {"PusherWall": "pusherWall", "Spinner": "spinner", "Cannon": "cannon", "Hammer": "hammer",
              "Trapdoor": "trapdoor", "Banana": "banana"}


def _sections(text):
    body = text.split("\n## Traps\n", 1)[1].split("\n## ", 1)[0]
    return {b.splitlines()[0].strip(): b for b in body.split("\n### ")[1:]}


def _field(block, name):
    m = re.search(r"^- \*\*" + re.escape(name) + r":\*\*(.*)$", block, re.M)
    return m.group(1).strip() if m else ""


def _catalogue():
    """Code name -> (cost, [(setting, min, max, default), ...]) from trapCatalogue()."""
    out = {}
    for m in re.finditer(r'^\s*\{TrapType::\w+, "(\w+)", (\d+), (.*)$', SOURCE.read_text(), re.M):
        settings = re.findall(r'\{"(\w+)", (\d+), (\d+), (\d+)\}', m.group(3))
        out[m.group(1)] = (int(m.group(2)), settings)
    return out


class TrapsDocTest(unittest.TestCase):
    def setUp(self):
        self.sections = _sections(DOC.read_text())
        self.catalogue = _catalogue()

    def test_seven_sections_no_empty_fields(self):
        self.assertEqual(len(self.sections), 7)
        for title, block in self.sections.items():
            for f in FIELDS:
                self.assertTrue(_field(block, f), f"{title}: empty or missing {f}")

    def test_setting_names_in_source(self):
        names = {s[0] for _, settings in self.catalogue.values() for s in settings}
        for title, block in self.sections.items():
            for f in ("Setting 1", "Setting 2"):
                self.assertIn(_field(block, f).split(",")[0], names, f"{title} {f}")

    def test_mapped_types_match_catalogue(self):
        for title, block in self.sections.items():
            code = _field(block, "Code type").split()[0]
            if code not in CODE_TYPES:
                continue
            cost, settings = self.catalogue[CODE_TYPES[code]]
            self.assertEqual(int(_field(block, "Cost")), cost, title)
            for i, f in enumerate(("Setting 1", "Setting 2")):
                name, lo, hi, dflt = settings[i]
                self.assertTrue(_field(block, f).startswith(f"{name}, {lo}\u2013{hi}, default {dflt}"), f"{title} {f}")


if __name__ == "__main__":
    unittest.main()
