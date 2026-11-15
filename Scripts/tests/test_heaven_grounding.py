"""Heaven props meant to stand on the cloud have the bottom of their fitted bounds on the floor top (1 cm),
the podium slab sits a hair above it (no z-fighting), floaters stay up, and dress_heaven places in cinema
space so a cinema actor away from world Z 0 still gets props on its cloud."""
import json
import math
import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))
import layout_spawn as S  # noqa: E402

LAYOUT = json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8"))
BOUNDS = json.loads((SCRIPTS / "tests/data/mesh_bounds.json").read_text(encoding="utf-8"))["HeavenBounds"]
CINEMA_Z = 37.0   # the cinema actor (and its cloud floor) need not stand at world Z 0


def world_bottom(piece, floor_z):
    lo, hi = BOUNDS[piece["mesh"]]
    target = (piece["at"][0], piece["at"][1], floor_z + piece["at"][2])
    _, _, loc, rlo, _ = S.fitted_location(lo, hi, piece["fit"], target, piece["yaw"])
    return loc[2] + rlo[2]


class HeavenGrounding(unittest.TestCase):
    def setUp(self):
        self.pieces = S.heaven_pieces(LAYOUT)
        self.floor_z = CINEMA_Z + S.FLOOR_TOP
        self.assertEqual(S.FLOOR_TOP, BOUNDS["SM_Heaven_Cloud_floor_top"])

    def test_grounded_bottoms_on_floor(self):
        grounded = [p for p in self.pieces if p["grounded"]]
        self.assertGreaterEqual(len(grounded), 10)
        for p in grounded:
            self.assertAlmostEqual(world_bottom(p, self.floor_z), self.floor_z, delta=1.0, msg=p["label"])

    def test_podium_slab_above_floor(self):
        podium = next(p for p in self.pieces if p["label"] == "Podium")
        gap = world_bottom(podium, self.floor_z) - self.floor_z
        self.assertTrue(0.1 <= gap <= 1.0, gap)

    def test_floaters_left_floating(self):
        floating = {p["label"] for p in self.pieces if not p["grounded"]}
        self.assertIn("Halo", floating)
        halo = next(p for p in self.pieces if p["label"] == "Halo")
        self.assertGreater(world_bottom(halo, self.floor_z), self.floor_z + 200.0)

    def test_pivot_offset_is_not_used(self):
        # A mesh whose pivot is 50 cm above its bottom still lands bottom-on-floor.
        _, _, loc, rlo, _ = S.fitted_location((-10, -10, -50), (10, 10, 50), ("height", 100.0), (0, 0, 5.0), 0.0)
        self.assertTrue(math.isclose(loc[2] + rlo[2], 5.0))

    def test_dress_heaven_places_in_cinema_space(self):
        src = (SCRIPTS / "dress_heaven.py").read_text(encoding="utf-8")
        self.assertIn("def mesh_bounds", (SCRIPTS / "layout_spawn.py").read_text(encoding="utf-8"))
        self.assertIn("cinema_point(origin, piece[\"at\"])", src)
        self.assertNotIn("unreal.Vector(spot[\"x\"], spot[\"y\"], 0.0)", src)


if __name__ == "__main__":
    unittest.main()
