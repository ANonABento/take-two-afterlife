import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import backyard_layout as L  # noqa: E402


class LayoutTests(unittest.TestCase):
    def test_corridor_comes_from_core(self):
        self.assertEqual((L.CORRIDOR_LENGTH, L.CORRIDOR_WIDTH), (60.0, 8.0))

    def test_layout_is_valid(self):
        L.validate()

    def test_route_reads_start_to_gate(self):
        self.assertLess(L.START.x, 2.0)
        self.assertGreater(L.GATE.x, L.CORRIDOR_LENGTH - 2.0)
        markers = sorted(p.x for p in L.props() if p.role == "marker")
        self.assertGreaterEqual(len(markers), 10)
        self.assertTrue(all(b - a == 4 for a, b in zip(markers, markers[1:])))

    def test_designed_dressing(self):
        dressing = [p for p in L.props() if p.role == "dressing"]
        self.assertGreaterEqual(len(dressing), 40)
        self.assertGreaterEqual(len({p.mesh for p in dressing}), 10)

    def test_dressing_in_corridor_is_rejected(self):
        bad = L.Prop("KenneyNature", "rock_smallA", "dressing", 30.0, 1.0, 0.0, 0, "Ink", ("height", 0.4))
        original = L._dressing
        L._dressing = lambda: original() + [bad]
        try:
            with self.assertRaisesRegex(ValueError, "intrudes"):
                L.validate()
        finally:
            L._dressing = original

    def test_unknown_palette_rejected(self):
        with self.assertRaises(ValueError):
            L.palette_index("Beige")

    def test_credits_list_every_asset(self):
        text = (L.REPO / "CREDITS.md").read_text(encoding="utf-8")
        self.assertEqual(L.credits_missing(text), [])

    def test_credits_check_catches_a_missing_mesh(self):
        text = (L.REPO / "CREDITS.md").read_text(encoding="utf-8").replace("`Coin_A`", "")
        self.assertIn(("KayKitPrototypeBits", "Coin_A"), L.credits_missing(text))
