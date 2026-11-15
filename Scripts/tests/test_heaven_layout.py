import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import toon_spec  # noqa: E402

SCRIPTS = Path(__file__).resolve().parents[1]
LAYOUT = json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8"))
ART = SCRIPTS.parent / "Content" / "Art" / "Heaven"


def _box(e):
    hx, hy = e["half"]
    return (e["x"] - hx, e["x"] + hx, e["y"] - hy, e["y"] + hy)


def _overlap(a, b):
    return a[0] < b[1] and b[0] < a[1] and a[2] < b[3] and b[2] < a[3]


class HeavenLayout(unittest.TestCase):
    def setUp(self):
        self.dressing = LAYOUT["podium_dressing"] + LAYOUT["spectator_ring"]

    def test_has_podium_dressing_and_ring(self):
        self.assertGreaterEqual(len(LAYOUT["podium_dressing"]), 4)
        self.assertGreaterEqual(len(LAYOUT["spectator_ring"]), 6)

    def test_no_dressing_overlaps_protected_or_each_other(self):
        for e in self.dressing:
            for p in LAYOUT["protected"]:
                self.assertFalse(_overlap(_box(e), _box(p)), (e["label"], p["label"]))
        for i, a in enumerate(self.dressing):
            for b in self.dressing[i + 1:]:
                self.assertFalse(_overlap(_box(a), _box(b)), (a["label"], b["label"]))

    def test_meshes_exist_and_colours_are_palette(self):
        names = [n for n, _ in toon_spec.PALETTE]
        for e in self.dressing:
            self.assertTrue((ART / f"{e['mesh']}.uasset").is_file(), e["mesh"])
            self.assertIn(e["colour"], names)

    def test_labels_unique(self):
        labels = [e["label"] for e in self.dressing]
        self.assertEqual(len(labels), len(set(labels)))


if __name__ == "__main__":
    unittest.main()
