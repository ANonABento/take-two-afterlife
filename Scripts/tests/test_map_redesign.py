"""Map fixes of Nov 15: kit units, Heaven sight line and podium, Backyard ground. No Unreal needed."""
import json
import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

import backyard_layout as L  # noqa: E402
import kit_obj  # noqa: E402
import layout_spawn as S  # noqa: E402

LAYOUT = json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8"))
PILLAR = """o Primitive_Pillar
v 0.5 0.0 0.5
v 0.5 4.0 0.5
v -0.5 0.0 -0.5
v -0.5 4.0 -0.5
vn 0.0 1.0 0.0
vt 0.5 0.5
f 1/1/1 2/1/1 3/1/1
"""


def _box(e):
    hx, hy = e["half"]
    return (e["x"] - hx, e["x"] + hx, e["y"] - hy, e["y"] + hy)


def _overlap(a, b):
    return a[0] < b[1] and b[0] < a[1] and a[2] < b[3] and b[2] < a[3]


class KitObj(unittest.TestCase):
    def test_metres_y_up_becomes_centimetres_z_up(self):
        # As shipped, UE reads the 4 m pillar as 4 cm long, lying along -Y.
        self.assertEqual(kit_obj.ue_bounds(PILLAR), ((-0.5, -4.0, -0.5), (0.5, 0.0, 0.5)))
        lo, hi = kit_obj.ue_bounds(kit_obj.transform_obj(PILLAR, 100.0))
        self.assertEqual((lo, hi), ((-50.0, -50.0, 0.0), (50.0, 50.0, 400.0)))

    def test_normals_turn_but_keep_length_and_faces_stay(self):
        out = kit_obj.transform_obj(PILLAR, 100.0)
        self.assertIn("vn 0.000000 -0.000000 1.000000", out)
        self.assertIn("vt 0.5 0.5", out)
        self.assertIn("f 1/1/1 2/1/1 3/1/1", out)

    def test_stage_copies_materials_and_skips_unity(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp) / "kit" / "obj", Path(tmp) / "staged"
            (src / "(Unity)").mkdir(parents=True)
            (src / "Primitive_Pillar.obj").write_text(PILLAR, encoding="utf-8")
            (src / "Primitive_Pillar.mtl").write_text("newmtl m\n", encoding="utf-8")
            (src / "tex.png").write_bytes(b"png")
            (src / "(Unity)" / "Other.obj").write_text(PILLAR, encoding="utf-8")
            staged = kit_obj.stage(Path(tmp) / "kit", dst, 100.0)
            self.assertEqual([p.name for p in staged], ["Primitive_Pillar.obj"])
            self.assertEqual(sorted(p.name for p in dst.iterdir()), ["Primitive_Pillar.mtl", "Primitive_Pillar.obj", "tex.png"])

    def test_import_kits_stages_the_obj_only_kit_and_places_nothing(self):
        src = (SCRIPTS / "import_kits.py").read_text(encoding="utf-8")
        self.assertIn("kit_obj.stage(", src)
        self.assertIn("KayKitPrototypeBits", kit_obj.OBJ_FIX)
        # The old retarget moved each StaticMeshActor's root component: every prop ended up at the origin.
        self.assertNotIn("set_relative_location", src)
        self.assertNotIn("load_level", src)

    def test_material_swaps_use_set_material(self):
        for name in ("swap_to_toon.py", "import_kits.py"):
            src = (SCRIPTS / name).read_text(encoding="utf-8")
            self.assertIn(".set_material(index, ", src, name)
        # swap_to_toon writes the array back only for skeletal meshes.
        self.assertIn('if prop != "static_materials":', (SCRIPTS / "swap_to_toon.py").read_text(encoding="utf-8"))


class HeavenSightLine(unittest.TestCase):
    def setUp(self):
        self.prot = S.heaven_protected(LAYOUT)
        self.view = _box(self.prot["ScreenView"])

    def test_view_box_runs_from_couch_to_screen(self):
        couch, screen = self.prot["Couch"], self.prot["Screen"]
        self.assertLessEqual(self.view[0], couch["x"] + couch["half"][0] + 50.0)
        self.assertGreaterEqual(self.view[1], screen["x"] - screen["half"][0])
        self.assertGreaterEqual(self.view[3], couch["half"][1])

    def test_nothing_stands_between_couch_and_screen(self):
        for label in ("Podium", "Gate", "RedCarpet", "PlayerStarts"):
            self.assertFalse(_overlap(_box(self.prot[label]), self.view), label)
        for piece in S.heaven_pieces(LAYOUT):
            x, y = piece["at"][:2]
            self.assertFalse(self.view[0] < x < self.view[1] and self.view[2] < y < self.view[3], piece["label"])

    def test_podium_and_gate_are_on_opposite_sides(self):
        self.assertLess(self.prot["Podium"]["y"], -400.0)
        self.assertGreater(self.prot["Gate"]["y"], 400.0)

    def test_everything_is_on_the_flat_of_the_cloud(self):
        # SM_Heaven_Cloud: flat disc of radius 15 m, rim puffs reach in to about 12.4 m.
        for piece in S.heaven_pieces(LAYOUT):
            if piece["label"].startswith("SM_Heaven_Cloud_"):
                continue                                    # far sky clouds
            x, y = piece["at"][:2]
            self.assertLess((x * x + y * y) ** 0.5, 1200.0, piece["label"])


class PodiumSpots(unittest.TestCase):
    def setUp(self):
        self.spots = S.podium_spots(LAYOUT)
        self.podium = S.heaven_protected(LAYOUT)["Podium"]

    def test_one_spot_per_step_inside_the_podium(self):
        self.assertEqual(set(self.spots), {"1st", "2nd", "3rd"})
        box = _box(self.podium)
        for tag, (x, y, _z, _yaw) in self.spots.items():
            self.assertTrue(box[0] < x < box[1] and box[2] < y < box[3], tag)
        xs = sorted(round(s[0]) for s in self.spots.values())
        self.assertEqual([b - a for a, b in zip(xs, xs[1:])], [150, 150])

    def test_statue_stands_on_its_step_winner_highest(self):
        z = {tag: s[2] for tag, s in self.spots.items()}
        self.assertGreater(z["1st"], z["2nd"])
        self.assertGreater(z["2nd"], z["3rd"])
        for tag, (_x, top) in S.PODIUM_STEPS.items():
            self.assertAlmostEqual(z[tag] - S.STATUE_HALF_HEIGHT, top)

    def test_statues_face_the_lobby(self):
        # Podium on the -Y side: the statues look towards +Y (yaw 90), at the couch side of the cloud.
        self.assertEqual({s[3] for s in self.spots.values()}, {90.0})

    def test_halo_floats_clear_of_the_winner_and_does_not_block(self):
        halo = next(p for p in S.heaven_pieces(LAYOUT) if p["label"] == "Halo")
        first = self.spots["1st"]
        self.assertEqual(halo["at"][:2], first[:2])
        self.assertGreater(halo["at"][2], first[2] + S.STATUE_HALF_HEIGHT)
        self.assertFalse(halo["collide"])

    def test_podium_keeps_its_proportions(self):
        podium = next(p for p in S.heaven_pieces(LAYOUT) if p["label"] == "Podium")
        self.assertEqual(S.fit_scale(S.PODIUM_SIZE, podium["fit"]), (1.0, 1.0, 1.0))


class BackyardGround(unittest.TestCase):
    def setUp(self):
        self.props = L.props()
        self.floors = [p for p in self.props if p.role == "floor"]

    def _covered(self, x, y):
        return any(abs(p.x - x) <= p.fit[1][0] / 2 + 1e-6 and abs(p.y - y) <= p.fit[1][1] / 2 + 1e-6
                   for p in self.floors)

    def test_every_dressing_piece_stands_on_ground(self):
        for p in self.props:
            if p.role == "dressing":
                self.assertTrue(self._covered(p.x, p.y), p)

    def test_trenches_are_open_beside_the_corridor_and_closed_at_the_ends(self):
        for x in (1.0, 30.0, 59.0):
            for side in (-1, 1):
                self.assertFalse(self._covered(x, side * (L.HALF_WIDTH + L.DITCH / 2)), (x, side))
        for x in (-1.0, L.CORRIDOR_LENGTH + 1.0):
            self.assertTrue(self._covered(x, L.HALF_WIDTH + L.DITCH / 2))

    def test_ground_is_solid_down_to_the_water(self):
        for p in self.floors:
            self.assertEqual(p.z + p.fit[1][2], 0.0, p)
            self.assertLess(p.z, L.WATER_Z, p)
        water = [p for p in self.props if p.role == "backdrop"]
        self.assertTrue(water)
        for p in water:
            self.assertGreater(p.z, L.KILL_PLANE.max_z)
            self.assertTrue(L.HALF_WIDTH < abs(p.y) < L.HALF_WIDTH + L.DITCH)

    def test_kill_plane_is_under_the_whole_block(self):
        k = L.KILL_PLANE
        for p in self.floors:
            hx, hy = p.fit[1][0] / 2, p.fit[1][1] / 2
            self.assertTrue(k.min_x < p.x - hx and p.x + hx < k.max_x and k.min_y < p.y - hy and p.y + hy < k.max_y, p)

    def test_fence_panels_touch(self):
        fences = sorted(p.x for p in self.props if p.mesh == "fence" and p.yaw == 0 and p.y > 0)
        self.assertTrue(all(b - a <= 2.12 for a, b in zip(fences, fences[1:])))

    def test_garden_has_its_landmarks(self):
        meshes = {p.mesh for p in self.props}
        for name in ("SM_Backyard_Shed", "SM_Backyard_Trampoline", "SM_Backyard_KiddiePool", "SM_Backyard_Grill",
                     "SM_Backyard_PicnicTable", "SM_Backyard_Gnome", "SM_Course_GateFinish"):
            self.assertIn(name, meshes)

    def test_build_script_keeps_start_gate_and_kill_plane_in_step(self):
        src = (SCRIPTS / "build_backyard_map.py").read_text(encoding="utf-8")
        for call in ("gate.set_actor_location(", "start.set_actor_location(", "kill.set_actor_location(",
                     "light_sky(actors)", "real_time_capture"):
            self.assertIn(call, src)
        self.assertNotIn("first_build", src)


if __name__ == "__main__":
    unittest.main()
