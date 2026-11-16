import ast, pathlib, re, unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "Source/TakeTwoAfterlife/Private"


def body(text, sig):
    start = text.index(sig)
    i = text.index("{", start)
    depth = 0
    for j in range(i, len(text)):
        depth += {"{": 1, "}": -1}.get(text[j], 0)
        if depth == 0:
            return text[i:j + 1]
    raise AssertionError(sig)


class ResolutionClampTests(unittest.TestCase):
    """LogTTAMenu: Resolution choice 17 out of range (17 choices)."""

    def test_apply_resolution_clamps_through_core(self):
        b = body((SRC / "TTAMenuWidget.cpp").read_text(), "bool UTTAMenuWidget::ApplyResolution(")
        self.assertIn("tta::clampResolutionIndex(", b)
        self.assertNotIn("out of range", b)
        self.assertNotIn("ResolutionChoices[ChoiceIndex]", b)

    def test_core_clamp_is_zero_based(self):
        h = (ROOT / "Core/include/tta/MenuFlow.h").read_text()
        self.assertIn("int clampResolutionIndex(int index, int count)", h)
        tests = (ROOT / "Core/tests/MenuFlowTests.cpp").read_text()
        self.assertIn("clampResolutionIndex(17, 17) == 16", tests)


class MaterialGuardTests(unittest.TestCase):
    """TTARunnerPawn CharacterMesh0: Material index 0/1 invalid in CreateAndSetMaterialInstanceDynamic."""

    def setUp(self):
        self.cpp = (SRC / "TTARunnerPawn.cpp").read_text()

    def test_body_mids_need_mesh_and_real_slot(self):
        b = body(self.cpp, "void ATTARunnerPawn::BeginPlay()")
        before = b[:b.index("GetMesh()->CreateAndSetMaterialInstanceDynamic(Slot)")]
        for need in ("GetSkeletalMeshAsset()", "GetNumMaterials()", "GetMaterial(Slot)"):
            self.assertIn(need, before)

    def test_hat_has_no_dynamic_material(self):
        """Hats use their painted materials as imported; an unguarded MID on the hat logged the same error."""
        self.assertNotIn("Hat->CreateAndSetMaterialInstanceDynamic", self.cpp)

    def test_runner_mesh_is_sk_angel(self):
        ini = (ROOT / "Config/DefaultGame.ini").read_text()
        sec = ini.split("[/Script/TakeTwoAfterlife.TTARunnerPawn]")[1].split("\n[")[0]
        self.assertIn("RunnerMesh=/Game/Characters/Angel/SK_Angel.SK_Angel", sec)


class BlendModeScriptTests(unittest.TestCase):
    """KayKit prototype_texture is Translucent on Nanite meshes (Barrel_A)."""

    def setUp(self):
        self.path = ROOT / "Scripts/fix_prototype_material.py"
        self.src = self.path.read_text()
        ast.parse(self.src)

    def test_sets_masked(self):
        for s in ("unreal.BlendMode.BLEND_MASKED", "prototype_texture", "save_asset"):
            self.assertIn(s, self.src)

    def test_runs_against_stub(self):
        import sys, types
        log = []

        class Mat:
            def __init__(self):
                self.p = {"blend_mode": "BLEND_TRANSLUCENT"}
            def get_editor_property(self, k): return self.p[k]
            def set_editor_property(self, k, v): self.p[k] = v
        mat = Mat()
        u = types.SimpleNamespace()
        u.Material = Mat
        u.MaterialInstanceConstant = type("MIC", (), {})
        u.BlendMode = types.SimpleNamespace(BLEND_MASKED="BLEND_MASKED", BLEND_TRANSLUCENT="BLEND_TRANSLUCENT")
        u.EditorAssetLibrary = types.SimpleNamespace(
            list_assets=lambda root, recursive=True, include_folder=False: [root + "/prototype_texture.prototype_texture", root + "/Barrel_A.Barrel_A"],
            load_asset=lambda p: mat if "prototype_texture" in p else object(),
            save_asset=lambda p, only_if_is_dirty=False: log.append(("save", p)) or True)
        u.MaterialEditingLibrary = types.SimpleNamespace(recompile_material=lambda m: log.append("recompile"))
        u.log = u.log_warning = lambda *a: None
        sys.modules["unreal"] = u
        try:
            ns = {"__name__": "not_main"}
            exec(compile(self.src, str(self.path), "exec"), ns)
            changed = ns["fix_prototype_materials"]()
        finally:
            del sys.modules["unreal"]
        self.assertEqual(mat.p["blend_mode"], "BLEND_MASKED")
        self.assertEqual(len(changed), 1)
        self.assertIn("recompile", log)


class AutomationDocTests(unittest.TestCase):
    def test_devlog_lists_run(self):
        d = (ROOT / "DEVLOG.md").read_text()
        self.assertRegex(d, r"`Automation RunTests TTA\.`[^\n]*MenuFlow[^\n]*pass")

    def test_menu_flow_uses_initialized_game_instance(self):
        t = (SRC / "Tests/TTAUIMenuFlowTest.cpp").read_text()
        self.assertIn("InitializeStandalone(", t)
        self.assertNotIn("UWorld::CreateWorld(", t)


if __name__ == "__main__":
    unittest.main()
