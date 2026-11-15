# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_kits.py -unattended -nosplash
# Imports every unique .fbx/.glb/.obj under Downloads/<kit> into /Game/ThirdParty/<kit>
# (FBX over GLB over OBJ, static mesh, combined, collision on). Default or untextured
# materials become an M_TTA_Toon palette instance. Kits in kit_obj.OBJ_FIX ship metres, Y-up OBJ only:
# they are staged through kit_obj.stage() so they arrive upright and in centimetres.
# Optional args: the kit keys to import (default: all three). Placing meshes is build_backyard_map.py's job.
import os
import re
import sys
from pathlib import Path

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import backyard_layout as L  # noqa: E402
import kit_obj  # noqa: E402
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CM = 100.0
RANK = {".fbx": 0, ".glb": 1, ".obj": 2}
BATCH = 20


def asset_name(stem):
    name = re.sub(r"[^A-Za-z0-9_]", "_", stem)
    if not name or name[0].isdigit():
        name = "SM_" + name
    return name


def choose_sources(kit_dir):
    """One file per stem. FBX wins, then GLB, then OBJ. Unity exports are skipped."""
    best = {}
    if not kit_dir.is_dir():
        return []
    for path in kit_dir.rglob("*"):
        if not path.is_file():
            continue
        rank = RANK.get(path.suffix.lower())
        if rank is None or "(unity)" in str(path).lower():
            continue
        key = path.stem.lower()
        current = best.get(key)
        if current is None or (rank, len(path.parts)) < current[0]:
            best[key] = ((rank, len(path.parts)), path)
    return [item[1] for item in best.values()]


def fbx_options():
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.import_animations = False
    ui.import_materials = True
    ui.import_textures = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    data = ui.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = True
    data.generate_lightmap_u_vs = True
    data.import_uniform_scale = 1.0
    return ui


def import_kit(kit, files):
    dest = f"/Game/ThirdParty/{kit}"
    imported = 0
    tasks = []
    for src in files:
        name = asset_name(src.stem)
        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        if src.suffix.lower() == ".fbx":
            task.options = fbx_options()
        tasks.append(task)
        if len(tasks) >= BATCH:
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
            imported += len(tasks)
            tasks = []
            unreal.log(f"{kit}: imported {imported}/{len(files)}")
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        imported += len(tasks)
    return dest


def used_textures(material):
    try:
        return list(MEL.get_used_textures(material))
    except Exception:
        return []


def is_plain(material):
    """Default engine material, or a material with no textures."""
    if material is None:
        return True
    path = material.get_path_name()
    if path.startswith("/Engine/") or path.startswith(toon_spec.TOON_MATERIAL + "."):
        return path.startswith("/Engine/")
    if isinstance(material, unreal.MaterialInstance):
        if material.get_editor_property("texture_parameter_values"):
            return False
    return not used_textures(material)


def palette_instance(colour, toon):
    path = f"{toon_spec.INSTANCE_DIR}/MI_TTA_Toon_Pal_{colour}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        path.rsplit("/", 1)[1], toon_spec.INSTANCE_DIR, unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew())
    if mi is None:
        raise RuntimeError(f"could not create {path}")
    MEL.set_material_instance_parent(mi, toon)
    MEL.set_material_instance_scalar_parameter_value(mi, "UsePaletteUV", 0.0)
    MEL.set_material_instance_scalar_parameter_value(mi, "PaletteIndex", float(L.palette_index(colour)))
    EAL.save_loaded_asset(mi)
    return mi


def paint_plain_slots(mesh, bone):
    slots = mesh.get_editor_property("static_materials")
    changed = False
    if not slots:
        slot = unreal.StaticMaterial()
        slot.set_editor_property("material_interface", bone)
        slot.set_editor_property("material_slot_name", "Palette")
        mesh.set_editor_property("static_materials", [slot])
        return True
    # StaticMesh.set_material, not a write-back of the static_materials array: that one did not stick.
    for index, slot in enumerate(slots):
        if is_plain(slot.get_editor_property("material_interface")):
            mesh.set_material(index, bone)
            changed = True
    return changed


def collision_count(mesh):
    body = mesh.get_editor_property("body_setup")
    if body is None:
        return 0
    agg = body.get_editor_property("agg_geom")
    total = 0
    for prop in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems", "tapered_capsule_elems"):
        try:
            total += len(agg.get_editor_property(prop))
        except Exception:
            pass
    return total


def ensure_collision(mesh):
    if collision_count(mesh) > 0:
        return
    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)


def finish_meshes(dest, bone):
    painted = 0
    for path in EAL.list_assets(dest, recursive=True, include_folder=False):
        asset = EAL.load_asset(path)
        if not isinstance(asset, unreal.StaticMesh):
            continue
        if paint_plain_slots(asset, bone):
            painted += 1
        ensure_collision(asset)
        EAL.save_loaded_asset(asset)
    return painted


def main():
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    bone = palette_instance("Bone", toon)
    downloads = L.downloads_dir([])
    kits = sys.argv[1:] or list(L.KITS)
    total = 0
    for kit in kits:
        if kit in kit_obj.OBJ_FIX:
            staging = L.REPO / "Saved" / "KitStaging" / kit
            files = kit_obj.stage(downloads / kit, staging, kit_obj.OBJ_FIX[kit])
        else:
            files = choose_sources(downloads / kit)
        if not files:
            raise RuntimeError(f"no meshes under {downloads / kit}")
        unreal.log(f"{kit}: {len(files)} unique meshes")
        dest = import_kit(kit, files)
        painted = finish_meshes(dest, bone)
        total += len(files)
        unreal.log(f"{kit}: painted {painted} plain meshes")
    EAL.save_directory("/Game/ThirdParty", only_if_is_dirty=False, recursive=True)
    unreal.log(f"imported {total} kit meshes")


main()
