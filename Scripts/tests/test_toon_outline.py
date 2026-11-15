"""Static checks on the toon/outline material builder (create_toon_assets.py)."""
import ast
import os
import unittest

SRC = os.path.join(os.path.dirname(__file__), "..", "create_toon_assets.py")


def _source():
    with open(SRC, encoding="utf-8") as f:
        return f.read()


def _const(name):
    for node in ast.parse(_source()).body:
        if isinstance(node, ast.Assign) and any(getattr(t, "id", None) == name for t in node.targets):
            return node.value.value
    raise AssertionError(f"{name} not found")


def _func(name):
    src = _source()
    for node in ast.parse(src).body:
        if isinstance(node, ast.FunctionDef) and node.name == name:
            return ast.get_source_segment(src, node)
    raise AssertionError(f"{name} not found")


class OutlineSamplingTest(unittest.TestCase):
    def test_lookups_use_buffer_uv(self):
        hlsl = _const("OUTLINE_HLSL")
        self.assertIn("ViewportUVToBufferUV(UV)", hlsl)
        lookups = [l for l in hlsl.splitlines() if "SceneTextureLookup(" in l]
        self.assertTrue(lookups)
        for line in lookups:
            self.assertIn("SceneTextureLookup(buv", line)

    def test_step_is_buffer_pixel(self):
        step = next(l for l in _const("OUTLINE_HLSL").splitlines() if l.startswith("float2 px"))
        self.assertIn("BufferSizeAndInvSize", step)
        self.assertNotIn("ViewSizeAndInvSize", step)


class ToonShadingModelTest(unittest.TestCase):
    def test_toon_is_lit_with_black_base(self):
        body = _func("make_toon")
        self.assertNotIn("MSM_UNLIT", body)
        self.assertIn("MSM_DEFAULT_LIT", body)
        self.assertIn("MP_BASE_COLOR", body)
        self.assertIn("MP_EMISSIVE_COLOR", body)


if __name__ == "__main__":
    unittest.main()
