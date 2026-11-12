# Run inside the UE editor (PythonScriptPlugin). Builds the toon look under /Game/Art/Toon:
# T_TTA_Palette, MPC_TTA_World (course values; UTTAWorldLookLibrary::ApplyWorldLook switches at runtime),
# M_TTA_Toon, M_TTA_PP_Outline + MI_TTA_PP_Ghost / MI_TTA_PP_CCTV, and the TTA_ArtTest map that
# TTA.Art.AllToon loads. Re-running rebuilds the graphs in place.
import os
import sys
import tempfile

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import toon_spec  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

# Unlit cel shading: banded N.L against MPC SunDirection, world grading, rim, emissive flash override.
TOON_HLSL = """
float3 n = normalize(N); float3 l = normalize(L); float3 v = normalize(V);
float ndl = saturate(dot(n, l) * 0.5 + 0.5);
float bands = clamp(round(Bands), 2, 3);
float lit = saturate(floor(ndl * bands) / (bands - 1));
float3 c = lerp(dot(Albedo, float3(0.299, 0.587, 0.114)).xxx, Albedo, Saturation);
c = saturate((c - 0.5) * Contrast + 0.5);
c *= lerp(ShadowFloor, 1.0, lit);
c *= lerp(float3(1, 1, 1), float3(1.08, 1.0, 0.88), Warmth);
float rim = pow(1 - saturate(dot(n, v)), 3) * RimStrength * step(0.5, ndl);
return lerp(c + rim, float3(4, 4, 4), saturate(Flash));
"""

# 4-tap edge detect on relative depth and world normal; ink where either crosses its threshold.
# Ghost and CCTV are blend weights, set to 1 by the MI_TTA_PP_Ghost / MI_TTA_PP_CCTV variants.
OUTLINE_HLSL = """
float2 px = View.ViewSizeAndInvSize.zw * Thickness;
float d = SceneTextureLookup(UV, 1, false).r;
float3 n = SceneTextureLookup(UV, 8, false).rgb;
float dEdge = 0; float nEdge = 0;
float2 o[4] = { float2(px.x, 0), float2(-px.x, 0), float2(0, px.y), float2(0, -px.y) };
for (int i = 0; i < 4; i++) {
  dEdge = max(dEdge, abs(SceneTextureLookup(UV + o[i], 1, false).r - d) / max(d, 1));
  nEdge = max(nEdge, 1 - dot(n, SceneTextureLookup(UV + o[i], 8, false).rgb));
}
float ink = saturate(step(DepthThreshold, dEdge) + step(NormalThreshold, nEdge)) * step(d, 100000);
float3 c = lerp(Scene, InkColor, ink);
float luma = dot(c, float3(0.299, 0.587, 0.114));
float3 ghost = lerp(c, luma * float3(0.75, 0.95, 1.15), 0.7) + 0.08;
float scan = 0.85 + 0.15 * sin(UV.y * View.ViewSizeAndInvSize.y * 1.5);
float2 cc = UV - 0.5; float vig = saturate(1 - dot(cc, cc) * 1.6);
float3 cctv = luma * float3(0.55, 1.0, 0.6) * scan * vig;
c = lerp(c, ghost, Ghost);
return lerp(c, cctv, CCTV);
"""


def ensure_asset(path, asset_class, factory):
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    folder, name = path.rsplit("/", 1)
    asset = TOOLS.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError(f"could not create {path}")
    return asset


def custom_node(material, code, inputs, x, y, output=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output)
    node.set_editor_property("inputs", [unreal.CustomInput(input_name=name) for name in inputs])
    return node


def scalar(material, name, value, x, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def connect(src, dst, pin, src_pin=""):
    if not MEL.connect_material_expressions(src, src_pin, dst, pin):
        raise RuntimeError(f"could not connect {src.get_name()} -> {pin}")


def make_palette():
    png = os.path.join(tempfile.mkdtemp(), "T_TTA_Palette.png")
    with open(png, "wb") as f:
        f.write(toon_spec.palette_png())
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", png)
    task.set_editor_property("destination_path", toon_spec.TOON_DIR)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    TOOLS.import_asset_tasks([task])
    tex = EAL.load_asset(toon_spec.PALETTE_TEXTURE)
    if tex is None:
        raise RuntimeError("palette import failed")
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("srgb", True)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    return tex


def make_mpc():
    mpc = ensure_asset(toon_spec.WORLD_MPC, unreal.MaterialParameterCollection,
                       unreal.MaterialParameterCollectionFactoryNew())
    course = toon_spec.load_world_presets()["Course"]
    params = []
    for name in toon_spec.WORLD_PARAMS:
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", course[name])
        params.append(p)
    mpc.set_editor_property("scalar_parameters", params)
    sun = unreal.CollectionVectorParameter()
    sun.set_editor_property("parameter_name", "SunDirection")
    sun.set_editor_property("default_value", unreal.LinearColor(0.4, 0.3, 0.87, 0))
    mpc.set_editor_property("vector_parameters", [sun])
    return mpc


def make_toon(palette, mpc):
    mat = ensure_asset(toon_spec.TOON_MATERIAL, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    # Palette colour: UV-indexed (meshes exported with palette UVs) or per-instance PaletteIndex.
    index = scalar(mat, "PaletteIndex", float(toon_spec.DEFAULT_INDEX), -1400, 0)
    use_uv = scalar(mat, "UsePaletteUV", 1.0, -1400, 100)
    uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 200)
    pick = custom_node(mat, f"return lerp(float2((Index + 0.5) / {len(toon_spec.PALETTE)}.0, 0.5), UV, UseUV);",
                       ["Index", "UseUV", "UV"], -1150, 100, unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    connect(index, pick, "Index")
    connect(use_uv, pick, "UseUV")
    connect(uv, pick, "UV")
    sample = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, 100)
    sample.set_editor_property("parameter_name", "Palette")
    sample.set_editor_property("texture", palette)
    connect(pick, sample, "UVs")

    toon = custom_node(mat, TOON_HLSL, ["Albedo", "N", "L", "V", "Flash", *toon_spec.WORLD_PARAMS], -400, 0)
    connect(sample, toon, "Albedo", "RGB")
    connect(MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, 300), toon, "N")
    connect(MEL.create_material_expression(mat, unreal.MaterialExpressionCameraVectorWS, -900, 400), toon, "V")
    for i, name in enumerate(("SunDirection", *toon_spec.WORLD_PARAMS)):
        node = MEL.create_material_expression(mat, unreal.MaterialExpressionCollectionParameter, -900, 500 + 100 * i)
        node.set_editor_property("collection", mpc)
        node.set_editor_property("parameter_name", name)
        connect(node, toon, "L" if name == "SunDirection" else name)
    connect(scalar(mat, "Flash", 0.0, -900, 1300), toon, "Flash")  # impact-frame white flash
    MEL.connect_material_property(toon, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    return mat


def make_outline():
    mat = ensure_asset(toon_spec.OUTLINE_MATERIAL, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)
    node = custom_node(mat, OUTLINE_HLSL, ["UV", "Scene", "Thickness", "DepthThreshold",
                                           "NormalThreshold", "InkColor", "Ghost", "CCTV"], -300, 0)
    connect(MEL.create_material_expression(mat, unreal.MaterialExpressionScreenPosition, -800, -200),
            node, "UV", "ViewportUV")
    scene = MEL.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -800, -50)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    connect(scene, node, "Scene", "Color")
    # Unconnected samplers so SceneTextureLookup can reach depth (1) and world normal (8) in the HLSL.
    for i, tex_id in enumerate((unreal.SceneTextureId.PPI_SCENE_DEPTH, unreal.SceneTextureId.PPI_WORLD_NORMAL)):
        extra = MEL.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -800, 450 + 120 * i)
        extra.set_editor_property("scene_texture_id", tex_id)
    for i, (name, value) in enumerate((("Thickness", 2.0), ("DepthThreshold", 0.02),
                                       ("NormalThreshold", 0.35), ("Ghost", 0.0), ("CCTV", 0.0))):
        connect(scalar(mat, name, value, -800, 100 + 70 * i), node, name)
    ink = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, 800)
    ink.set_editor_property("parameter_name", "InkColor")
    r, g, b = (c / 255 for c in toon_spec.palette_rgb()[0])
    ink.set_editor_property("default_value", unreal.LinearColor(r, g, b, 1))
    connect(ink, node, "InkColor")
    MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    for name, param in (("MI_TTA_PP_Ghost", "Ghost"), ("MI_TTA_PP_CCTV", "CCTV")):
        mi = ensure_asset(f"{toon_spec.TOON_DIR}/{name}", unreal.MaterialInstanceConstant,
                          unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, mat)
        MEL.set_material_instance_scalar_parameter_value(mi, param, 1.0)


def make_test_map(toon):
    """Three engine shapes, each with a palette-index instance."""
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(toon_spec.ART_TEST_MAP):
        level.load_level(toon_spec.ART_TEST_MAP)
    elif not level.new_level(toon_spec.ART_TEST_MAP):
        raise RuntimeError(f"could not create {toon_spec.ART_TEST_MAP}")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.StaticMeshActor):
            actors.destroy_actor(actor)
    for i, (shape, index) in enumerate((("Cube", 2), ("Sphere", 5), ("Cylinder", 8))):
        mi = ensure_asset(f"{toon_spec.INSTANCE_DIR}/MI_TTA_Toon_Test{shape}", unreal.MaterialInstanceConstant,
                          unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, toon)
        MEL.set_material_instance_scalar_parameter_value(mi, "UsePaletteUV", 0.0)
        MEL.set_material_instance_scalar_parameter_value(mi, "PaletteIndex", float(index))
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(i * 200.0, 0, 50))
        comp = actor.static_mesh_component
        comp.set_static_mesh(EAL.load_asset(f"/Engine/BasicShapes/{shape}"))
        comp.set_material(0, mi)
    if not level.save_current_level():
        raise RuntimeError(f"could not save {toon_spec.ART_TEST_MAP}")


def main():
    toon = make_toon(make_palette(), make_mpc())
    make_outline()
    if not EAL.save_directory(toon_spec.TOON_DIR, only_if_is_dirty=False, recursive=True):
        raise RuntimeError(f"could not save {toon_spec.TOON_DIR}")
    make_test_map(toon)
    EAL.save_directory(toon_spec.INSTANCE_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log("toon assets built")


main()
