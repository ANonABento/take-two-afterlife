import re
import sys
import types
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "Scripts"
SRC = ROOT / "Source" / "TakeTwoAfterlife"


class _Obj:
    """Any attribute or call returns another _Obj, so UE actor/component chains just work."""

    def __init__(self, kind="obj"):
        self.kind, self.props = kind, {}

    def set_editor_property(self, name, value):
        self.props[name] = value

    def __call__(self, *a, **k):
        return _Obj()

    def __getattr__(self, name):
        return _Obj(name)


def _fake_unreal(exists=True):
    u = types.ModuleType("unreal")
    spawned, world = [], _Obj("WorldSettings")
    for n in ["DirectionalLight", "SkyLight", "SkyAtmosphere", "ExponentialHeightFog", "CameraActor", "StaticMeshActor"]:
        setattr(u, n, n)
    u.Vector = lambda *a: a
    u.Rotator = lambda *a, **k: (a, k)
    u.log = lambda m: spawned.append(("log", m))
    u.load_class = lambda outer, path: path.rsplit(".", 1)[-1]

    class Actors:
        def spawn_actor_from_class(self, cls, *a):
            spawned.append(cls)
            return _Obj(cls)

        def get_all_level_actors(self):
            return ["OldSun", "OldCamera"]

        def destroy_actor(self, actor):
            spawned.append(("destroyed", actor))
            return True

    class EAL:
        does_asset_exist = staticmethod(lambda p: exists)
        load_asset = staticmethod(lambda p: _Obj("asset"))

    class Level:
        def new_level(self, p): return spawned.append(("new", p)) or True
        def load_level(self, p): return spawned.append(("loaded", p)) or True
        def save_current_level(self): return True

    class Editor:
        def get_editor_world(self):
            w = _Obj("World")
            w.get_world_settings = lambda: world
            return w

    subs = {"EditorActorSubsystem": Actors(), "LevelEditorSubsystem": Level(), "UnrealEditorSubsystem": Editor()}
    for k in subs:
        setattr(u, k, k)
    u.get_editor_subsystem = lambda k: subs[k]
    u.EditorAssetLibrary = EAL
    return u, spawned, world


class TitleMapTest(unittest.TestCase):
    def run_script(self, exists):
        u, spawned, world = _fake_unreal(exists)
        sys.modules["unreal"] = u
        try:
            exec(compile((SCRIPTS / "create_title_map.py").read_text(), "create_title_map.py", "exec"), {"__name__": "__main__"})
        finally:
            del sys.modules["unreal"]
        return spawned, world

    def test_title_world_uses_title_game_mode(self):
        _, world = self.run_script(exists=False)
        self.assertEqual(world.props.get("default_game_mode"), "TTATitleGameMode")

    def test_title_is_lit_and_has_a_camera(self):
        spawned, _ = self.run_script(exists=False)
        for cls in ["DirectionalLight", "SkyLight", "SkyAtmosphere", "ExponentialHeightFog", "CameraActor", "StaticMeshActor"]:
            self.assertIn(cls, spawned)

    def test_rerun_rebuilds_existing_title(self):
        spawned, world = self.run_script(exists=True)
        # Deleting the map and recreating it at the same path crashes the editor: reruns load and clear.
        self.assertNotIn("delete_asset", (SCRIPTS / "create_title_map.py").read_text().split("\nif EAL")[1])
        self.assertIn(("loaded", "/Game/Maps/Title"), spawned)
        self.assertNotIn(("new", "/Game/Maps/Title"), spawned)
        self.assertIn(("destroyed", "OldSun"), spawned)
        self.assertIn(("destroyed", "OldCamera"), spawned)
        self.assertEqual(world.props.get("default_game_mode"), "TTATitleGameMode")
        self.assertIn(("log", "created /Game/Maps/Title"), spawned)

    def test_title_mode_has_no_build_hud_or_pawn(self):
        cpp = (SRC / "Private" / "TTATitleGameMode.cpp").read_text()
        self.assertIn("DefaultPawnClass = nullptr", cpp)
        self.assertIn("ATTATitlePlayerController::StaticClass()", cpp)
        self.assertNotIn("TTABuildHUD", cpp)

    def test_global_mode_is_gameplay_and_backyard_overrides_explicitly(self):
        ini = (ROOT / "Config" / "DefaultEngine.ini").read_text()
        self.assertIn("GlobalDefaultGameMode=/Script/TakeTwoAfterlife.TTAGameMode", ini)
        self.assertIn("/Script/TakeTwoAfterlife.TTAGameMode", (SCRIPTS / "build_backyard_map.py").read_text())

    def test_host_travels_to_backyard(self):
        h = (SRC / "Public" / "TTASessionSubsystem.h").read_text()
        self.assertIn('LobbyMap = TEXT("/Game/Maps/Heaven")', h)

    def test_travel_shows_a_loading_screen(self):
        cpp = (SRC / "Private" / "TTASessionSubsystem.cpp").read_text()
        self.assertIn("PreLoadMap", cpp)
        self.assertIn("SetupLoadingScreen", cpp)
        self.assertIn("LOADING", cpp)
        self.assertIn("MoviePlayer", (SRC / "TakeTwoAfterlife.Build.cs").read_text())


class TitleBackdropScriptTest(unittest.TestCase):
    def test_backdrop_stands_up_and_is_saved(self):
        create = (SCRIPTS / "create_title_map.py").read_text()
        apply = (SCRIPTS / "apply_title_backdrop.py").read_text()
        # Rotator is (roll, pitch, yaw) positionally; a bare (0, 0, 90) left the plane lying flat.
        self.assertNotRegex(create + apply, r"Rotator\(\s*[-\d*]")
        self.assertIn('plane("Backdrop", (2600, 0, 400), (72, 48, 1), roll=90.0, yaw=90.0)', create)
        self.assertNotIn('plane("Ground"', create)
        self.assertIn("unreal.Rotator(roll=90.0, pitch=0.0, yaw=90.0)", apply)
        # The painting has to be saved into the map, not just set on the open level.
        self.assertLess(apply.index("set_material(0, mi)"), apply.index("level.save_current_level()"))


if __name__ == "__main__":
    unittest.main()
