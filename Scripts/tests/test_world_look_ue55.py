import ast
import re
import sys
import types
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))
import toon_spec  # noqa: E402

BANNED = re.compile(r"unreal\.(Kismet\w*|EditorLevelLibrary)\b")


class _Param:
    def __init__(self, name, value):
        self.props = {"parameter_name": name, "default_value": value}

    def get_editor_property(self, name):
        return self.props[name]

    def set_editor_property(self, name, value):
        self.props[name] = value


class _MPC:
    def __init__(self, scalars, vectors=()):
        self.props = {"scalar_parameters": [_Param(n, 0.0) for n in scalars],
                      "vector_parameters": [_Param(n, None) for n in vectors]}

    def get_editor_property(self, name):
        return list(self.props[name])

    def set_editor_property(self, name, value):
        self.props[name] = list(value)


def _load(name, unreal):
    tree = ast.parse((SCRIPTS / "apply_world_look.py").read_text())
    fn = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == name)
    ns = {"unreal": unreal, "toon_spec": toon_spec, "EAL": unreal.EditorAssetLibrary}
    exec(compile(ast.Module(body=[fn], type_ignores=[]), "apply_world_look.py", "exec"), ns)
    return ns[name]


class UE55ApiTests(unittest.TestCase):
    def test_no_removed_api_in_scripts(self):
        hits = [f"{p.name}:{i}" for p in SCRIPTS.rglob("*.py") if p.parent.name != "tests"
                for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1) if BANNED.search(line)]
        self.assertEqual(hits, [])

    def test_editor_world_from_subsystem(self):
        src = (SCRIPTS / "apply_world_look.py").read_text()
        self.assertIn("get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()", src)

    def test_every_look_map_gets_volume(self):
        self.assertEqual(set(toon_spec.LOOK_MAPS), {"/Game/Maps/Title", "/Game/Maps/Backyard", "/Game/Maps/Heaven"})
        src = (SCRIPTS / "apply_world_look.py").read_text()
        self.assertRegex(src, r"for map_path, preset in toon_spec\.LOOK_MAPS\.items\(\):\n(.*\n){0,3}.*place_volume\(")


class MpcDefaultsTests(unittest.TestCase):
    def setUp(self):
        self.saved = []
        self.unreal = types.SimpleNamespace(
            EditorAssetLibrary=types.SimpleNamespace(save_asset=lambda p, only_if_is_dirty=True: self.saved.append(p) or True),
            LinearColor=lambda *a: a)

    def test_writes_course_defaults_and_saves(self):
        mpc = _MPC(toon_spec.WORLD_PARAMS, ["SunDirection"])
        _load("write_mpc_defaults", self.unreal)(mpc, toon_spec.load_world_presets())
        course = toon_spec.load_world_presets()["Course"]
        got = {p.props["parameter_name"]: p.props["default_value"] for p in mpc.props["scalar_parameters"]}
        self.assertEqual(got, course)
        self.assertEqual(self.saved, [toon_spec.WORLD_MPC])

    def test_missing_parameter_fails_loudly(self):
        mpc = _MPC(toon_spec.WORLD_PARAMS[:-1])
        with self.assertRaises(RuntimeError):
            _load("write_mpc_defaults", self.unreal)(mpc, toon_spec.load_world_presets())


if __name__ == "__main__":
    unittest.main()
