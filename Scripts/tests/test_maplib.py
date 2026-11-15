import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import backyard_layout as L  # noqa: E402
import maplib  # noqa: E402

GOLDEN = Path(__file__).resolve().parent / "data" / "backyard_golden.json"


def _plain(value):
    return json.loads(json.dumps(value))


class BackyardMatchesOldScript(unittest.TestCase):
    """Req 1: Backyard rebuilt on maplib places exactly what the pre-maplib script placed."""

    @classmethod
    def setUpClass(cls):
        cls.old = json.loads(GOLDEN.read_text(encoding="utf-8"))

    def test_props_identical(self):
        self.assertEqual(_plain([list(p) for p in L.props()]), self.old["props"])

    def test_start_gate_kill_identical(self):
        self.assertEqual(_plain(list(L.START)), self.old["start"])
        self.assertEqual(_plain(list(L.GATE)), self.old["gate"])
        self.assertEqual(_plain(list(L.KILL_PLANE)), self.old["kill_plane"])
        self.assertEqual(L.WORLD_KILL_Z, self.old["world_kill_z"])
        self.assertEqual(_plain(list(L.GATE_TRIGGER_HALF)), self.old["gate_trigger_half"])
        self.assertEqual([L.CORRIDOR_LENGTH, L.CORRIDOR_WIDTH], self.old["corridor"])

    def test_zones_kits_meshes_identical(self):
        self.assertEqual(_plain([list(z) for z in L.ZONES]), self.old["zones"])
        self.assertEqual(_plain({k: list(v) for k, v in L.KITS.items()}), self.old["kits"])
        self.assertEqual(L.meshes_by_kit(), self.old["meshes"])


class MaplibRules(unittest.TestCase):
    def test_grid_from_core(self):
        self.assertEqual(maplib.grid_meters(), 0.5)

    def test_unknown_role_rejected(self):
        bad = L.Prop("KenneyNature", "rock_smallA", "trap", -30.0, 0.0, 0.0, 0, "Ink", ("height", 0.4))
        with self.assertRaisesRegex(ValueError, "unknown role"):
            maplib.validate(L.props() + [bad], L.KITS, L.CORRIDOR_LENGTH, L.CORRIDOR_WIDTH, L.START, L.GATE,
                            L.KILL_PLANE, L.DITCH, L.LAWN_WIDTH, L.FLOOR_THICKNESS)

    def test_floor_gap_rejected(self):
        props = [p for p in L.props() if not (p.role == "floor" and p.palette == "Yellow")]
        with self.assertRaisesRegex(ValueError, "gap at x = 24.0"):
            maplib.validate(props, L.KITS, L.CORRIDOR_LENGTH, L.CORRIDOR_WIDTH, L.START, L.GATE, L.KILL_PLANE,
                            L.DITCH, L.LAWN_WIDTH, L.FLOOR_THICKNESS)

    def test_kill_plane_too_high_rejected(self):
        high = maplib.kill_plane(L.CORRIDOR_LENGTH, min_z=-3.0, max_z=-1.0)
        with self.assertRaisesRegex(ValueError, "below the floor"):
            maplib.validate(L.props(), L.KITS, L.CORRIDOR_LENGTH, L.CORRIDOR_WIDTH, L.START, L.GATE, high,
                            L.DITCH, L.LAWN_WIDTH, L.FLOOR_THICKNESS)


if __name__ == "__main__":
    unittest.main()
