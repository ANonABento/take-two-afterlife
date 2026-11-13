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
    u.Vector = u.Rotator = lambda *a: a
    u.log = lambda m: spawned.append(("log", m))
    u.load_class = lambda outer, path: path.rsplit(".", 1)[-1]

    class Actors:
        def spawn_actor_from_class(self, cls, *a):
            spawned.append(cls)
            return _Obj(cls)

    class EAL:
        does_asset_exist = staticmethod(lambda p: exists)
        delete_asset = staticmethod(lambda p: spawned.append(("deleted", p)) or True)
        load_asset = staticmethod(lambda p: _Obj("asset"))

    class Level:
        def new_level(self, p): return True
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
        self.assertIn(("deleted", "/Game/Maps/Title"), spawned)
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
        self.assertIn('LobbyMap = TEXT("/Game/Maps/Backyard")', h)


if __name__ == "__main__":
    unittest.main()
