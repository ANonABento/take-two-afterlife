"""Trap size + pivot check (trap-model-swap task 2): models/trap_bounds.json vs the placeholder exports."""
import sys
import unittest
from pathlib import Path

MODELS = Path(__file__).resolve().parents[2] / "models"
sys.path.insert(0, str(MODELS))
import trap_fit  # noqa: E402
import check_credits  # noqa: E402

BOUNDS = trap_fit.load_bounds()
PLACEHOLDERS = {k: v for k, v in trap_fit.glb_bounds(str(MODELS / "traps_cams.glb")).items() if k.startswith("SM_Trap_")}
ORIGIN = (0.0, 0.0, 0.0)


class TrapBoundsTests(unittest.TestCase):
    def test_every_placeholder_has_a_box_and_fits(self):
        self.assertTrue(PLACEHOLDERS)
        for name, (mn, mx) in PLACEHOLDERS.items():
            with self.subTest(name=name):
                self.assertEqual(trap_fit.check_trap(name, mn, mx, ORIGIN, BOUNDS), [])

    def test_lawnmower_box(self):
        box = BOUNDS["SM_Trap_Lawnmower"]
        self.assertEqual([box["max_cm"][i] - box["min_cm"][i] for i in range(3)], [100.0, 60.0, 50.0])
        self.assertEqual(box["min_cm"][2], 0.0)  # pivot at the deck base

    def test_scaled_2x_fails(self):
        for name, (mn, mx) in PLACEHOLDERS.items():
            with self.subTest(name=name):
                self.assertTrue(trap_fit.check_trap(name, [v * 2 for v in mn], [v * 2 for v in mx], ORIGIN, BOUNDS))

    def test_shifted_pivot_fails(self):
        mn, mx = PLACEHOLDERS["SM_Trap_Hammer"]
        self.assertTrue(trap_fit.check_trap("SM_Trap_Hammer", mn, mx, (0.0, 0.0, 30.0), BOUNDS))
        dz = (mx[2] - mn[2]) * 0.5  # mesh moved off its hinge, origin unchanged
        self.assertTrue(trap_fit.check_trap("SM_Trap_Hammer", [mn[0], mn[1], mn[2] + dz], [mx[0], mx[1], mx[2] + dz], ORIGIN, BOUNDS))

    def test_unknown_trap_fails(self):
        self.assertTrue(trap_fit.check_trap("SM_Trap_Nope", [0, 0, 0], [1, 1, 1], ORIGIN, BOUNDS))

    def test_verify_script_uses_check(self):
        self.assertIn("trap_fit.check_trap", (MODELS / "verify_traps_cams.py").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()


def _apply(rows, v):
    return [sum(r[i] * v[i] for i in range(3)) + r[3] for r in rows]


def _det(rows):
    (a, b, c), (d, e, f), (g, h, i) = (r[:3] for r in rows)
    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)


class TrapImportFitTests(unittest.TestCase):
    """Import fit for the CC0 downloads (trap-model-swap task 4): rotate to motion axes, pivot, scale to box."""

    def _fit(self, name, mn, mx):
        rows = trap_fit.fit_matrix(mn, mx, BOUNDS[name])
        corners = [_apply(rows, (x, y, z)) for x in (mn[0], mx[0]) for y in (mn[1], mx[1]) for z in (mn[2], mx[2])]
        lo = [min(c[i] for c in corners) for i in range(3)]
        hi = [max(c[i] for c in corners) for i in range(3)]
        return rows, lo, hi

    def test_task1_extents_land_in_their_boxes(self):
        # raw glTF extents from task 1 (m, Y-up) converted to Blender Z-up cm, off-centre on purpose
        raw = {"SM_Trap_Banana": (13.8, 63.0, 19.6), "SM_Trap_Trapdoor": (174.1, 157.3, 21.5),
               "SM_Trap_Spinner": (300.0, 19.8, 286.0), "SM_Trap_Hammer": (40.0, 30.0, 120.0)}
        for name, dims in raw.items():
            with self.subTest(name=name):
                mn = [7.0, -3.0, 11.0]
                rows, lo, hi = self._fit(name, mn, [mn[i] + dims[i] for i in range(3)])
                self.assertEqual(trap_fit.check_trap(name, lo, hi, ORIGIN, BOUNDS), [])
                self.assertGreater(_det(rows), 0)

    def test_upright_banana_lies_along_x(self):
        rows, lo, hi = self._fit("SM_Trap_Banana", [0, 0, 0], [13.8, 19.6, 63.0])
        self.assertAlmostEqual(abs(rows[0][2]), 55.4 / 63.0)  # tall source Z -> long box X

    def test_every_thirdparty_trap_has_box_palette_colour_and_credit(self):
        pal = trap_fit.palette()
        self.assertEqual(len(pal), 12)
        credits = (MODELS.parent / "CREDITS.md").read_text(encoding="utf-8")
        for name, (fname, url, colour) in trap_fit.THIRDPARTY.items():
            with self.subTest(name=name):
                self.assertIn(name, BOUNDS)
                self.assertIn(colour, pal)
                self.assertIn(f"`{fname}`", credits)
                self.assertTrue(url.startswith("https://static.poly.pizza/"))

    def test_uncredited_thirdparty_file_is_flagged(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "traps").mkdir()
            (root / "traps" / "mystery.glb").write_bytes(b"glTF")
            self.assertEqual(check_credits.thirdparty_problems("", root), ["no credits row: models/thirdparty/traps/mystery.glb"])
            self.assertEqual(check_credits.thirdparty_problems("| `mystery.glb` |", root), [])
