"""Layout placement: bounds fit, real spawn location, safe reruns (Backyard + Heaven). No Unreal needed."""
import json
import math
import re
import sys
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

import backyard_layout as L  # noqa: E402
import layout_spawn as S  # noqa: E402

CM = 100.0
CUBE = (100.0, 100.0, 100.0)   # /Engine/BasicShapes/Cube, the placeholder for any missing mesh


def backyard_fit(prop):
    kind, target = prop.fit
    return ("box", tuple(v * CM for v in target)) if kind == "box" else (kind, target * CM)


def heaven():
    return S.heaven_pieces(json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8")))


class FitScale(unittest.TestCase):
    def test_flat_carpet_is_not_blown_up_100x(self):
        # The reported bug: a 2 cm thick carpet came out at scale 100 and x = -11945.
        carpet = next(p for p in heaven() if p["label"] == "RedCarpet")
        scale = S.fit_scale((250.0, 1200.0, 2.0), carpet["fit"])       # SM_Heaven_RedCarpet: long along Y
        self.assertTrue(all(S.MIN_SCALE <= s <= S.MAX_SCALE for s in scale), scale)
        self.assertAlmostEqual(scale[0], 1.0)                           # full width, only shortened
        self.assertAlmostEqual(1200.0 * scale[1], 700.0)
        self.assertEqual(carpet["at"][:2], (150.0, 600.0))

    def test_kit_metres_read_as_centimetres_raise(self):
        # KayKit as UE imported its OBJ: a 4 m floor tile 4 cm wide. The build must stop, not scale by 150.
        with self.assertRaises(ValueError):
            S.fit_scale((4.0, 1.0, 4.0), ("box", (600.0, 800.0, 450.0)))
        S.fit_scale((400.0, 400.0, 100.0), ("box", (600.0, 800.0, 450.0)))

    def test_wrong_units_raise(self):
        with self.assertRaises(ValueError):
            S.fit_scale((1.0, 1.0, 1.0), ("height", 700.0))

    def test_every_entry_scale_in_range(self):
        for size in (CUBE,):
            for prop in L.props():
                if prop.kit == "TTAHeaven":
                    continue   # clouds are fitted up to 49 m wide: fine on their own meshes, not on a 1 m cube
                for s in S.fit_scale(size, backyard_fit(prop)):
                    self.assertTrue(0.05 <= s <= 10.0, (prop, size, s))
            for piece in heaven():
                for s in S.fit_scale(size, piece["fit"]):
                    self.assertTrue(0.05 <= s <= 10.0, (piece["label"], size, s))


class Placement(unittest.TestCase):
    def test_bottom_centre_lands_on_target(self):
        lo, hi, scale = (-10.0, 20.0, 5.0), (30.0, 60.0, 50.0), (2.0, 2.0, 2.0)
        for yaw in (0, 90, 180, 45):
            loc = S.placement((3000.0, 400.0, 0.0), yaw, lo, hi, scale)
            cx, cy = 10.0 * 2, 40.0 * 2
            r = math.radians(yaw)
            wx = loc[0] + cx * math.cos(r) - cy * math.sin(r)
            wy = loc[1] + cx * math.sin(r) + cy * math.cos(r)
            self.assertAlmostEqual(wx, 3000.0)
            self.assertAlmostEqual(wy, 400.0)
            self.assertAlmostEqual(loc[2] + lo[2] * 2, 0.0)

    def test_root_component_is_never_moved(self):
        src = (SCRIPTS / "build_backyard_map.py").read_text(encoding="utf-8")
        self.assertNotIn("static_mesh_component.set_relative_location", src)
        place = src[src.index("def place("):src.index("def build(")]
        self.assertNotIn("set_relative_location", place)
        heaven_src = (SCRIPTS / "dress_heaven.py").read_text(encoding="utf-8")
        self.assertNotIn("sensible_scale", heaven_src)
        self.assertNotIn("100.0, 100.0, 100.0", heaven_src)


class BackyardSpread(unittest.TestCase):
    def test_spans_20m(self):
        xs = [p.x for p in L.props()]
        self.assertGreaterEqual(max(xs) - min(xs), 20.0)

    def test_no_two_dressing_pieces_share_a_spot(self):
        seen = {}
        for p in L.props():
            if p.role != "dressing":
                continue
            key = (round(p.x, 2), round(p.y, 2))
            self.assertNotIn(key, seen, f"{p.mesh} on top of {seen.get(key)}")
            seen[key] = p.mesh

    def test_route_markers_run_start_to_gate(self):
        xs = [p.x for p in L.props() if p.role == "marker"]
        self.assertGreater(len(xs), 2)
        self.assertEqual(xs, sorted(xs))
        self.assertEqual(len(set(xs)), len(xs))
        self.assertGreater(xs[0], L.START.x)
        self.assertLess(xs[-1], L.GATE.x)


class FakeActor:
    def __init__(self, cls, tags):
        self.cls, self.tags = cls, tags

    def get_class(self):
        return type("C", (), {"get_name": lambda _s, c=self.cls: c})()

    def get_editor_property(self, name):
        return self.tags


class FakeSubsystem:
    def __init__(self, actors):
        self.actors = list(actors)

    def get_all_level_actors(self):
        return list(self.actors)

    def destroy_actor(self, actor):
        self.actors.remove(actor)


class FakeLabelled(FakeActor):
    def __init__(self, cls, tags, label):
        super().__init__(cls, tags)
        self.label = label

    def get_actor_label(self):
        return self.label


class LegacyPurge(unittest.TestCase):
    """Maps built before the TTA_Layout tag kept every old actor: two podiums, three suns, a 250 m carpet."""

    def test_untagged_leftovers_go_and_hand_placed_actors_stay(self):
        labels = S.heaven_labels(json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8")))
        old = [FakeLabelled("StaticMeshActor", [], "RedCarpet"), FakeLabelled("StaticMeshActor", [], "Podium"),
               FakeLabelled("TargetPoint", ["1st"], "Podium_1st")]
        keep = [FakeLabelled("StaticMeshActor", [], "HandPlacedRock"), FakeLabelled("PlayerStart", [], "Podium"),
                FakeLabelled("TTACinema", [], "Cinema")]
        sub = FakeSubsystem(old + keep)
        gone = S.delete_previous(sub, lambda cls, label: cls in ("StaticMeshActor", "TargetPoint") and label in labels)
        self.assertEqual(gone, 3)
        self.assertEqual(sub.actors, keep)

    def test_no_actor_is_read_after_a_destroy(self):
        """Reading actors after a destroy in the same pass crashed the editor on a backyard rebuild."""
        sub = FakeSubsystem([])
        destroy = sub.destroy_actor
        sub.destroy_actor = lambda actor: (setattr(sub, "destroyed", True), destroy(actor))

        class Watched(FakeLabelled):
            def get_class(self):
                if getattr(sub, "destroyed", False):
                    raise AssertionError("actor read after a destroy")
                return super().get_class()

        sub.actors = [Watched("StaticMeshActor", [], label) for label in ("RedCarpet", "Keep", "Podium")]
        gone = S.delete_previous(sub, lambda cls, label: label in ("RedCarpet", "Podium"))
        self.assertEqual(gone, 2)
        self.assertEqual([a.label for a in sub.actors], ["Keep"])

    def test_both_scripts_purge_legacy_actors(self):
        self.assertIn("S.delete_previous(actors, legacy)", (SCRIPTS / "build_backyard_map.py").read_text(encoding="utf-8"))
        self.assertRegex((SCRIPTS / "dress_heaven.py").read_text(encoding="utf-8"), r"S\.delete_previous\(subsystem, lambda")


class Rerun(unittest.TestCase):
    def test_deletes_only_old_layout_actors(self):
        keep = [FakeActor("PlayerStart", ["TTA_Layout"]), FakeActor("TTAFinishGate", []),
                FakeActor("TTAKillPlane", []), FakeActor("NavMeshBoundsVolume", ["TTA_Layout"]),
                FakeActor("StaticMeshActor", ["Hand"])]
        old = [FakeActor("StaticMeshActor", ["TTA_Layout", "TTARoute"]), FakeActor("TargetPoint", ["TTA_Layout"])]
        sub = FakeSubsystem(keep + old)
        self.assertEqual(S.delete_previous(sub), 2)
        self.assertEqual(sub.actors, keep)

    def test_scripts_delete_before_spawning_and_tag(self):
        for name in ("build_backyard_map.py", "dress_heaven.py"):
            src = (SCRIPTS / name).read_text(encoding="utf-8")
            body = src[src.index("def build(" if "backyard" in name else "def main("):]
            first_spawn = min(m.start() for m in re.finditer(r"spawn_(actor_from_class|mesh|fitted)\(|place\(", body))
            self.assertLess(body.index("S.delete_previous("), first_spawn, name)
            self.assertNotRegex(body, r"isinstance\(actor, \(unreal\.StaticMeshActor, unreal\.PlayerStart", name)
        self.assertIn("tag(unreal, actor", (SCRIPTS / "layout_spawn.py").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
