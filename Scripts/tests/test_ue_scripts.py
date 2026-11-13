import ast
import sys
import tempfile
import types
import unittest
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))


class _UStruct:
    """Mimics UE Python structs: the constructor takes no args (UE 5 raises this exact TypeError)."""

    def __init__(self, *args, **kwargs):
        if args or kwargs:
            raise TypeError(f"call() takes at most 0 arguments ({len(args) + len(kwargs)} given)")
        self.props = {}

    def set_editor_property(self, name, value):
        self.props[name] = value


def _load_function(script, name, unreal):
    tree = ast.parse((SCRIPTS / script).read_text())
    fn = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == name)
    ns = {"unreal": unreal, "MEL": unreal.MaterialEditingLibrary}
    exec(compile(ast.Module(body=[fn], type_ignores=[]), script, "exec"), ns)
    return ns[name]


class CustomNodeTest(unittest.TestCase):
    def test_custom_inputs_built_without_constructor_args(self):
        unreal = types.SimpleNamespace(
            CustomInput=type("CustomInput", (_UStruct,), {}),
            MaterialExpressionCustom=object,
            CustomMaterialOutputType=types.SimpleNamespace(CMOT_FLOAT3=3),
            MaterialEditingLibrary=types.SimpleNamespace(create_material_expression=lambda *a: _UStruct()),
        )
        custom_node = _load_function("create_toon_assets.py", "custom_node", unreal)
        node = custom_node(object(), "return N;", ["N", "L"], 0, 0)
        self.assertEqual([i.props["input_name"] for i in node.props["inputs"]], ["N", "L"])


class DefaultsTest(unittest.TestCase):
    def test_backyard_defaults_to_repo_downloads(self):
        import backyard_layout as L
        self.assertEqual(L.downloads_dir([]), L.REPO / "Downloads")
        self.assertEqual(L.downloads_dir(["x/y"]), Path("x/y"))

    def test_missing_pack_gives_none_not_error(self):
        import backyard_layout as L
        with tempfile.TemporaryDirectory() as d:
            fbx = Path(d) / "KenneyNature" / "Models" / "FBX format"
            fbx.mkdir(parents=True)
            (fbx / "tree_oak.fbx").write_bytes(b"")
            self.assertEqual(L.find_fbx(Path(d), "KenneyNature", "tree_oak").name, "tree_oak.fbx")
            self.assertIsNone(L.find_fbx(Path(d), "KenneyNature", "nope"))
            self.assertIsNone(L.find_fbx(Path(d), "KenneyCitySuburban", "fence"))

    def test_swap_defaults_to_game(self):
        self.assertIn('sys.argv[1:] or ["/Game"]', (SCRIPTS / "swap_to_toon.py").read_text())

    def test_setup_runs_the_four_scripts_in_order(self):
        text = (SCRIPTS / "setup_content.ps1").read_text()
        order = ["create_toon_assets.py", "build_backyard_map.py", "swap_to_toon.py", "create_title_map.py"]
        self.assertEqual(sorted(order, key=text.index), order)
        self.assertIn("Traceback", text)

    def test_fetch_covers_every_layout_kit(self):
        import backyard_layout as L
        text = (SCRIPTS / "fetch_assets.ps1").read_text()
        for kit in L.KITS:
            self.assertIn(kit, text)


if __name__ == "__main__":
    unittest.main()
