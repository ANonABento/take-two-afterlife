"""Every Heaven prop players can reach blocks them: everything dress_heaven.py spawns except the halo, trophy
and carpet collides, spawn_fitted gives those actors BlockAll and a simple collision body (a kit mesh with none
lets runners walk through it), and the cinema screen (the TV) is made blocking instead of the C++ NoCollision."""
import json
import sys
import types
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))
import layout_spawn as S  # noqa: E402

LAYOUT = json.loads((SCRIPTS / "heaven_layout.json").read_text(encoding="utf-8"))
PASS_THROUGH = {"Halo", "Trophy", "RedCarpet"}


class Vec:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x, self.y, self.z = x, y, z


class Agg:
    def __init__(self, n):
        self.n = n

    def get_editor_property(self, name):
        return [object()] * self.n if name == "box_elems" else []


class Mesh:
    def __init__(self, simple):
        self.agg = Agg(simple)

    def get_bounding_box(self):
        return types.SimpleNamespace(min=Vec(-50, -50, 0), max=Vec(50, 50, 100))

    def get_editor_property(self, name):
        return types.SimpleNamespace(get_editor_property=lambda p: self.agg) if name == "body_setup" else None


class Comp:
    def __init__(self):
        self.profile, self.enabled = None, None

    def set_collision_profile_name(self, name):
        self.profile = name

    def set_collision_enabled(self, value):
        self.enabled = value


class Actor:
    def __init__(self):
        self.static_mesh_component = Comp()

    def __getattr__(self, name):
        return lambda *a, **k: None


def fake_unreal(added):
    def add(mesh, shape):
        added.append(shape)
        mesh.agg.n += 1
    return types.SimpleNamespace(
        Vector=Vec, Rotator=lambda *a: a, Name=str,
        CollisionEnabled=types.SimpleNamespace(NO_COLLISION="none", QUERY_AND_PHYSICS="all"),
        ScriptingCollisionShapeType=types.SimpleNamespace(BOX="box"),
        EditorStaticMeshLibrary=types.SimpleNamespace(add_simple_collisions=add),
        EditorAssetLibrary=types.SimpleNamespace(save_loaded_asset=lambda m: True))


class Sub:
    def spawn_actor_from_object(self, mesh, loc, rot):
        return Actor()


class HeavenCollision(unittest.TestCase):
    def test_every_prop_but_halo_trophy_carpet_collides(self):
        pieces = S.heaven_pieces(LAYOUT)
        self.assertEqual({p["label"] for p in pieces if not p["collide"]}, PASS_THROUGH)
        self.assertTrue(any(p["mesh"] == "SM_Heaven_VelvetRopeStanchions" and p["collide"] for p in pieces))

    def test_colliding_spawn_blocks_and_gets_simple_collision(self):
        added = []
        actor, _ = S.spawn_fitted(fake_unreal(added), Sub(), "Stanchion_L1", Mesh(0), ("height", 100.0),
                                  (0, 0, 0), 0.0)
        self.assertEqual(actor.static_mesh_component.profile, "BlockAll")
        self.assertEqual(added, ["box"])

    def test_mesh_with_collision_is_left_alone(self):
        added = []
        S.spawn_fitted(fake_unreal(added), Sub(), "Podium", Mesh(2), ("height", 100.0), (0, 0, 0), 0.0)
        self.assertEqual(added, [])

    def test_pass_through_spawn_has_no_collision(self):
        added = []
        actor, _ = S.spawn_fitted(fake_unreal(added), Sub(), "Halo", Mesh(0), ("height", 100.0), (0, 0, 0), 0.0,
                                  collide=False)
        self.assertEqual(actor.static_mesh_component.enabled, "none")
        self.assertEqual(added, [])

    def test_dress_heaven_makes_screen_and_couch_block(self):
        src = (SCRIPTS / "dress_heaven.py").read_text(encoding="utf-8")
        self.assertIn("for comp in (couch, screen):", src)
        self.assertIn("S.make_blocking(unreal, comp)", src)


if __name__ == "__main__":
    unittest.main()
