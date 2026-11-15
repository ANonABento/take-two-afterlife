"""Trap size + pivot check (trap-model-swap task 2): models/trap_bounds.json vs the placeholder exports."""
import sys
import unittest
from pathlib import Path

MODELS = Path(__file__).resolve().parents[2] / "models"
sys.path.insert(0, str(MODELS))
import trap_fit  # noqa: E402

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
