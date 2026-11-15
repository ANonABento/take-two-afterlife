# Gen 3 character assets -> Unreal, one shot. Run inside the UE 5.5 editor (Python Editor Script Plugin on):
#   Output Log > Cmd > Python:   py "Scripts/import_gen3_assets.py"
# Before running (PowerShell, repo root) so the gen 3 hats are on disk:
#   git checkout origin/plan/take-two-afterlife-mwgf7edt -- models/angel-v2/hats
# What it does:
#   1. imports models/SK_Angel.fbx (gen 3, 20 bones) to /Game/Characters/Angel/SK_Angel, new skeleton + physics asset
#   2. builds M_TTA_Angel (Tint + FaceTexture params, face UV scale/offset) and gives every slot an instance:
#      Body white #F2F0EC, Outfit blue #A9C6E3, Face = FaceTexture
#   3. makes the hat attach point the runner uses (HatSocket = "head"): a socket on the head_hat bone if no bone is named head
#   4. imports the 8 gen 3 hats as SM_Hat_<Name> in /Game/Characters/Hats (cm, scale checked)
#   5. rewrites DT_Cosmetics: the game's 6 hat slots (MenuFlow.h kHats) point at gen 3 hats via HAT_MAP; faces stay T_Face_*
#   6. saves and prints a report. Re-run any time; it replaces.
# Not done here: animations (old anims are on the old skeleton: retarget or make a new AnimBP), per-hat fine nudging.
import re
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[1]
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ANGEL = "/Game/Characters/Angel"
HATS = "/Game/Characters/Hats"
FACES = "/Game/Characters/Faces"
MAT_DIR = "/Game/Characters/Angel/Materials"
HAT_SRC = REPO / "models" / "angel-v2" / "hats"
GEN3_HATS = ["crown", "halo", "devil-horns", "propeller-beanie", "party-cone", "viking-helmet", "bunny-ears", "traffic-cone"]
# Game slot name (kHats order) -> gen 3 hat folder. Edit to taste, then re-run.
HAT_MAP = {"Chef": "crown", "Cowboy": "viking-helmet", "Firefighter": "propeller-beanie",
           "HardHat": "party-cone", "Pilot": "bunny-ears", "Police": "devil-horns"}
SLOT_COLOURS = {"body": (0.886, 0.871, 0.831), "outfit": (0.397, 0.564, 0.768)}  # linear of #F2F0EC / #A9C6E3
FACE_UV = (1.0, 1.0, 0.0, 0.0)  # scale u, scale v, offset u, offset v; tweak on MI_Angel_Face if the face sits wrong
HAT_Z_CM = 0.0                  # extra lift above the hat bone, per-row edits in DT_Cosmetics after


def camel(folder):
    return "".join(p.capitalize() for p in folder.split("-"))


def task(src, dest, name, opts):
    t = unreal.AssetImportTask()
    t.filename, t.destination_path, t.destination_name = str(src), dest, name
    t.automated, t.replace_existing, t.save, t.options = True, True, True, opts
    return t


def import_angel():
    src = REPO / "models" / "SK_Angel.fbx"
    if not src.is_file():
        raise RuntimeError(f"missing {src}")
    ui = unreal.FbxImportUI()
    ui.import_mesh, ui.import_as_skeletal, ui.import_animations = True, True, False
    ui.import_materials, ui.import_textures, ui.create_physics_asset = False, False, True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.skeleton = None  # gen 3 has its own bones; never reuse the old skeleton
    d = ui.skeletal_mesh_import_data
    d.import_uniform_scale, d.convert_scene, d.convert_scene_unit = 1.0, True, True
    d.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    TOOLS.import_asset_tasks([task(src, ANGEL, "SK_Angel", ui)])
    sk = EAL.load_asset(f"{ANGEL}/SK_Angel")
    if sk is None:
        raise RuntimeError("SK_Angel import failed")
    pa = EAL.load_asset(f"{ANGEL}/SK_Angel_PhysicsAsset")
    if pa and sk.get_editor_property("physics_asset") is None:
        sk.set_editor_property("physics_asset", pa)
    h = sk.get_bounds().box_extent.z * 2
    if h < 50:
        unreal.log_warning(f"SK_Angel only {h:.1f} cm tall: imported in metres, re-importing x100")
        d.import_uniform_scale = 100.0
        TOOLS.import_asset_tasks([task(src, ANGEL, "SK_Angel", ui)])
        sk = EAL.load_asset(f"{ANGEL}/SK_Angel")
    return sk


def angel_material():
    path = f"{MAT_DIR}/M_TTA_Angel"
    m = EAL.load_asset(path)
    if m is None:
        m = TOOLS.create_asset("M_TTA_Angel", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 0)
    tint.set_editor_property("parameter_name", "Tint")
    use = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 200)
    use.set_editor_property("parameter_name", "UseFace")
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1300, 400)
    su = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1300, 550)
    su.set_editor_property("parameter_name", "FaceUV")  # R,G scale  B,A offset
    su.set_editor_property("default_value", unreal.LinearColor(*FACE_UV))
    rg = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1100, 550)
    rg.set_editor_property("r", True); rg.set_editor_property("g", True)
    ba = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1100, 650)
    ba.set_editor_property("b", True); ba.set_editor_property("a", True)
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -950, 450)
    add = MEL.create_material_expression(m, unreal.MaterialExpressionAdd, -800, 450)
    tex = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 400)
    tex.set_editor_property("parameter_name", "FaceTexture")
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    blend = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -300, 100)
    alpha = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 300)
    MEL.connect_material_expressions(su, "", rg, ""); MEL.connect_material_expressions(su, "", ba, "")
    MEL.connect_material_expressions(uv, "", mul, "A"); MEL.connect_material_expressions(rg, "", mul, "B")
    MEL.connect_material_expressions(mul, "", add, "A"); MEL.connect_material_expressions(ba, "", add, "B")
    MEL.connect_material_expressions(add, "", tex, "UVs")
    MEL.connect_material_expressions(tex, "A", alpha, "A"); MEL.connect_material_expressions(use, "", alpha, "B")
    MEL.connect_material_expressions(tint, "", blend, "A"); MEL.connect_material_expressions(tex, "RGB", blend, "B")
    MEL.connect_material_expressions(alpha, "", blend, "Alpha")
    MEL.connect_material_property(blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 300)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m)
    return m


def instance(parent, name, colour, use_face):
    path = f"{MAT_DIR}/{name}"
    mi = EAL.load_asset(path) or TOOLS.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant,
                                                    unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*colour, 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "UseFace", 1.0 if use_face else 0.0)
    face = EAL.load_asset(f"{FACES}/T_Face_Default") or next(
        (EAL.load_asset(p) for p in EAL.list_assets(FACES, False) if "T_Face_" in p), None)
    if face:
        MEL.set_material_instance_texture_parameter_value(mi, "FaceTexture", face)
    EAL.save_loaded_asset(mi)
    return mi


def assign_slots(sk, parent):
    mats = sk.get_editor_property("materials")
    report = []
    for i, slot in enumerate(mats):
        n = str(slot.get_editor_property("material_slot_name")).lower()
        if "face" in n:
            mi = instance(parent, "MI_Angel_Face", SLOT_COLOURS["body"], True)
        elif "outfit" in n or "sash" in n:
            mi = instance(parent, "MI_Angel_Outfit", SLOT_COLOURS["outfit"], False)
        else:
            mi = instance(parent, "MI_Angel_Body", SLOT_COLOURS["body"], False)
        slot.set_editor_property("material_interface", mi)
        mats[i] = slot
        report.append(f"slot {i} {n} -> {mi.get_name()}")
    sk.set_editor_property("materials", mats)
    EAL.save_loaded_asset(sk)
    return report


def bone_names(sk):
    comp = unreal.SkeletalMeshComponent()
    comp.set_skeletal_mesh_asset(sk)
    return [str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())]


def hat_attach(sk):
    """Runner attaches the hat to FName 'head' (TTARunnerPawn::HatSocket)."""
    bones = bone_names(sk)
    if "head" in bones:
        return "bone head"
    target = next((b for b in bones if b.lower() == "head_hat"), None) or next(
        (b for b in bones if b.lower() == "head"), None)
    if target is None:
        raise RuntimeError(f"no head bone in {bones}")
    if sk.find_socket("head") is None:
        s = unreal.SkeletalMeshSocket(outer=sk)
        s.set_editor_property("socket_name", "head")
        s.set_editor_property("bone_name", target)
        sk.add_socket(s, True)
        EAL.save_loaded_asset(sk)
    return f"socket head on {target}"


def import_hats():
    tasks = []
    for folder in GEN3_HATS:
        d = HAT_SRC / folder
        src = d / f"{folder}.fbx"
        if not src.is_file():
            src = d / f"{folder}.glb"
        if not src.is_file():
            raise RuntimeError(f"missing hat {d}: run the git checkout line at the top")
        name = f"SM_Hat_{camel(folder)}"
        if src.suffix == ".fbx":
            ui = unreal.FbxImportUI()
            ui.import_mesh, ui.import_as_skeletal, ui.import_materials, ui.import_textures = True, False, True, True
            ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
            sd = ui.static_mesh_import_data
            sd.combine_meshes, sd.convert_scene, sd.convert_scene_unit = True, True, True
            sd.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
            tasks.append(task(src, HATS, name, ui))
        else:
            tasks.append(task(src, HATS, name, None))
    TOOLS.import_asset_tasks(tasks)
    out = []
    for folder in GEN3_HATS:
        name = f"SM_Hat_{camel(folder)}"
        mesh = EAL.load_asset(f"{HATS}/{name}")
        if mesh is None:
            raise RuntimeError(f"{name} failed to import")
        size = mesh.get_bounding_box().max - mesh.get_bounding_box().min
        if max(size.x, size.y, size.z) < 5:
            unreal.log_warning(f"{name} is {size.z:.2f} cm tall (metres): reimport it with Uniform Scale 100")
        out.append(f"{name} {size.x:.0f}x{size.y:.0f}x{size.z:.0f} cm")
    return out


def game_hat_slots():
    text = (REPO / "Core" / "include" / "tta" / "MenuFlow.h").read_text(encoding="utf-8")
    hats = re.findall(r'"([^"]+)"', re.search(r"kHats\s*=\s*\{([^}]*)\}", text).group(1))
    faces = re.findall(r'"([^"]+)"', re.search(r"kFaces\s*=\s*\{([^}]*)\}", text).group(1))
    return hats, faces


def rebuild_table():
    import sys
    sys.path.insert(0, str(REPO / "Scripts"))
    import cosmetics_rows as C
    hats, faces = game_hat_slots()
    rows = C.build_rows(hats, faces)
    for r, hat in zip(rows, hats):
        name = f"SM_Hat_{camel(HAT_MAP[hat])}"
        r["HatMesh"] = f"{HATS}/{name}.{name}"
        r["SocketOffset"] = (0.0, 0.0, HAT_Z_CM)
    struct = unreal.load_object(None, C.ROW_STRUCT)
    if struct is None:
        raise RuntimeError("compile the game module first (TTACosmeticRow missing)")
    path = f"{C.TABLE_DIR}/{C.TABLE_NAME}"
    table = EAL.load_asset(path)
    if table is None:
        f = unreal.DataTableFactory(); f.set_editor_property("struct", struct)
        table = TOOLS.create_asset(C.TABLE_NAME, C.TABLE_DIR, unreal.DataTable, f)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, C.to_csv(rows)):
        raise RuntimeError("DT_Cosmetics CSV fill failed")
    EAL.save_asset(path)
    missing = [r["FaceTexture"] for r in rows if not EAL.does_asset_exist(r["FaceTexture"].split(".")[0])]
    return [f"{r['Name']}: {h} -> {r['HatMesh'].split('.')[-1]}" for r, h in zip(rows, hats)], missing


def main():
    sk = import_angel()
    report = [f"SK_Angel {sk.get_bounds().box_extent.z * 2:.0f} cm tall"]
    report += assign_slots(sk, angel_material())
    report.append("hat attach: " + hat_attach(sk))
    report += import_hats()
    rows, missing_faces = rebuild_table()
    report += rows
    if missing_faces:
        report.append(f"MISSING faces (run Scripts/build_cosmetics_table.py once): {missing_faces}")
    for folder in (ANGEL, HATS, "/Game/Data"):
        EAL.save_directory(folder, only_if_is_dirty=False, recursive=True)
    unreal.log("\n".join(["=== gen 3 import ==="] + report))


main()
