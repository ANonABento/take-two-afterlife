"""Kit meshes in the wrong units / axes: unit fix, axis match, skip-not-abort. No Unreal needed."""
import json
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

import backyard_layout as L  # noqa: E402
import layout_spawn as S  # noqa: E402

CM = 100.0
BOUNDS = json.loads((SCRIPTS / "tests/data/mesh_bounds.json").read_text(encoding="utf-8"))


def backyard_fit(prop):
    kind, target = prop.fit
    return ("box", tuple(v * CM for v in target)) if kind == "box" else (kind, target * CM)


def in_guard(scale):
    return all(S.MIN_SCALE <= s <= S.MAX_SCALE for s in scale)


class UnitsAndAxes(unittest.TestCase):
    def test_reported_floor_still_fails_raw(self):
        # The build log: Primitive_Floor bounds [4, 1, 4] against the 6 x 8 x 0.2 m patio tile.
        with self.assertRaises(ValueError):
            S.fit_scale([4, 1, 4], ("box", (600.0, 800.0, 20.0)))

    def test_floor_fits_with_unit_fix_and_x_roll(self):
        scale, rot, guard = S.fit_mesh([4, 1, 4], ("box", (600.0, 800.0, 20.0)))
        self.assertEqual(rot, (90.0, 0.0))
        self.assertTrue(in_guard(guard), guard)
        lo, hi = S.rotated_box((-2, -0.5, -2), (2, 0.5, 2), scale, rot)
        for got, want in zip((h - l for h, l in zip(hi, lo)), (600.0, 800.0, 20.0)):
            self.assertAlmostEqual(got, want)

    def test_flat_z_up_slab_is_left_alone(self):
        scale, rot, guard = S.fit_mesh([400, 400, 20], ("box", (600.0, 800.0, 20.0)))
        self.assertEqual(rot, (0.0, 0.0))
        self.assertEqual(scale, guard)

    def test_cube_placeholder_never_rotates(self):
        _, rot, _ = S.fit_mesh((100.0, 100.0, 100.0), ("box", (600.0, 800.0, 20.0)))
        self.assertEqual(rot, (0.0, 0.0))

    def test_every_layout_entry_against_recorded_bounds(self):
        checked = 0
        for prop in L.props():
            if prop.mesh not in BOUNDS.get(prop.kit, {}):
                continue  # kit without recorded bounds yet (e.g. TTAProps)
            size = BOUNDS[prop.kit][prop.mesh]
            _, _, guard = S.fit_mesh(size, backyard_fit(prop))
            self.assertTrue(all(0.05 <= s <= 10.0 for s in guard), (prop.mesh, size, guard))
            checked += 1
        heaven = S.heaven_pieces(json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8")))
        for piece in heaven:
            size = BOUNDS["Heaven"][piece["mesh"]]
            _, _, guard = S.fit_mesh(size, piece["fit"])
            self.assertTrue(all(0.05 <= s <= 10.0 for s in guard), (piece["label"], size, guard))
            checked += 1
        self.assertGreater(checked, 50)


def vec(*v):
    return SimpleNamespace(x=v[0], y=v[1], z=v[2])


class FakeUnreal:
    def __init__(self):
        self.warnings = []
        self.Vector = lambda *a: a
        self.Rotator = lambda *a: a
        self.Name = str

    def log_warning(self, msg):
        self.warnings.append(msg)


class Spawned:
    def __init__(self):
        self.props = {}

    def __getattr__(self, name):
        return lambda *a, **k: self.props.setdefault(name, a)


class FakeSubsystem:
    def __init__(self):
        self.spawned = []

    def spawn_actor_from_object(self, mesh, loc, rot):
        actor = Spawned()
        self.spawned.append((actor, rot))
        return actor


def mesh(size):
    return SimpleNamespace(get_bounding_box=lambda: SimpleNamespace(min=vec(0, 0, 0), max=vec(*size)))


class SkipBadProp(unittest.TestCase):
    def test_bad_prop_is_skipped_with_warning(self):
        ue, sub = FakeUnreal(), FakeSubsystem()
        actor, size = S.spawn_or_skip(ue, sub, "floor_Bad", mesh((1.0, 1.0, 1000.0)), ("box", (600.0, 800.0, 20.0)),
                                      (0.0, 0.0, 0.0), 0.0)
        self.assertIsNone(actor)
        self.assertEqual(sub.spawned, [])
        self.assertEqual(len(ue.warnings), 1)
        self.assertIn("floor_Bad", ue.warnings[0])

    def test_reported_floor_spawns_rolled(self):
        ue, sub = FakeUnreal(), FakeSubsystem()
        actor, size = S.spawn_or_skip(ue, sub, "floor_Primitive_Floor", mesh((4, 1, 4)),
                                      ("box", (600.0, 800.0, 20.0)), (300.0, 0.0, -20.0), 0.0)
        self.assertIsNotNone(actor)
        self.assertEqual(sub.spawned[0][1], (90.0, 0.0, 0.0))
        for got, want in zip(size, (600.0, 800.0, 20.0)):
            self.assertAlmostEqual(got, want)
        self.assertEqual(ue.warnings, [])

    def test_build_scripts_skip_instead_of_abort(self):
        for name in ("build_backyard_map.py", "dress_heaven.py"):
            src = (SCRIPTS / name).read_text(encoding="utf-8")
            self.assertIn("S.spawn_or_skip(", src, name)
            self.assertNotIn("S.spawn_fitted(", src, name)


if __name__ == "__main__":
    unittest.main()
