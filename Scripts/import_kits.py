# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_kits.py -unattended -nosplash
# Imports every unique .fbx/.glb/.obj under Downloads/<kit> into /Game/ThirdParty/<kit>
# (FBX over GLB over OBJ, static mesh, combined, collision on). Default or untextured
# materials become an M_TTA_Toon palette instance. Backyard dressing cubes for the garden
# props keep their actor names and transforms; the mesh and scale are fitted to the layout.
import os
import re
import sys
from pathlib import Path

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import backyard_layout as L  # noqa: E402
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CM = 100.0
RANK = {".fbx": 0, ".glb": 1, ".obj": 2}
# Layout mesh names for the garden props the Backyard cubes stand in for.
DRESSING = {
    "fence", "planter", "tree-large", "tree-small", "plant_bushLarge",
    "flower_redA", "flower_yellowA", "rock_smallA", "Barrel_A", "Box_A", "Box_B",
    "Pallet_Small", "table_medium",
}
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
    for slot in slots:
        old = slot.get_editor_property("material_interface")
        if is_plain(old):
            slot.set_editor_property("material_interface", bone)
            changed = True
    if changed:
        mesh.set_editor_property("static_materials", slots)
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


def mesh_asset(prop):
    name = asset_name(prop.mesh)
    path = f"/Game/ThirdParty/{prop.kit}/{name}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    raw = f"/Game/ThirdParty/{prop.kit}/{prop.mesh}"
    if EAL.does_asset_exist(raw):
        return EAL.load_asset(raw)
    return None


def fit(component, mesh, prop):
    box = mesh.get_bounding_box()
    size = box.max - box.min
    size = unreal.Vector(max(size.x, 0.01), max(size.y, 0.01), max(size.z, 0.01))
    kind, target = prop.fit
    if kind == "box":
        scale = unreal.Vector(target[0] * CM / size.x, target[1] * CM / size.y, target[2] * CM / size.z)
    else:
        s = target * CM / size.z
        scale = unreal.Vector(s, s, s)
    centre = unreal.Vector((box.min.x + box.max.x) / 2 * scale.x, (box.min.y + box.max.y) / 2 * scale.y,
                           box.min.z * scale.z)
    component.set_relative_scale3d(scale)
    component.set_relative_location(unreal.Vector(-centre.x, -centre.y, -centre.z), False, False)


def retarget_backyard(toon):
    """Swap dressing cubes for kit meshes. Actor label, location and rotation stay."""
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not EAL.does_asset_exist(L.MAP_PATH):
        raise RuntimeError(f"{L.MAP_PATH} missing")
    level.load_level(L.MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    statics = [a for a in actors if isinstance(a, unreal.StaticMeshActor)]
    used = set()
    swapped = 0
    missing = []
    for prop in L.props():
        if prop.mesh not in DRESSING:
            continue
        mesh = mesh_asset(prop)
        if mesh is None:
            missing.append(f"{prop.kit}/{prop.mesh}")
            continue
        want = f"{prop.role}_{prop.mesh}"
        target = unreal.Vector(prop.x * CM, prop.y * CM, prop.z * CM)
        best = None
        best_score = None
        for index, actor in enumerate(statics):
            if index in used:
                continue
            label = actor.get_actor_label()
            dist = (actor.get_actor_location() - target).length()
            if label != want and dist > 25.0:
                continue
            score = (0 if label == want else 1, dist)
            if best is None or score < best_score:
                best, best_score = index, score
        if best is None:
            missing.append(f"actor {want}")
            continue
        used.add(best)
        actor = statics[best]
        if actor.get_actor_label() != want:
            actor.set_actor_label(want)
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        fit(comp, mesh, prop)
        colour = palette_instance(prop.palette, toon)
        for slot in range(max(comp.get_num_materials(), 1)):
            comp.set_material(slot, colour)
        swapped += 1
    if not level.save_current_level():
        raise RuntimeError(f"could not save {L.MAP_PATH}")
    return swapped, missing


def main():
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    bone = palette_instance("Bone", toon)
    downloads = L.downloads_dir([])
    total = 0
    for kit in ("KayKitPrototypeBits", "KenneyCitySuburban", "KenneyNature"):
        files = choose_sources(downloads / kit)
        if not files:
            raise RuntimeError(f"no meshes under {downloads / kit}")
        unreal.log(f"{kit}: {len(files)} unique meshes")
        dest = import_kit(kit, files)
        painted = finish_meshes(dest, bone)
        total += len(files)
        unreal.log(f"{kit}: painted {painted} plain meshes")
    swapped, missing = retarget_backyard(toon)
    EAL.save_directory("/Game/ThirdParty", only_if_is_dirty=False, recursive=True)
    if missing:
        for item in missing:
            unreal.log_error(f"backyard retarget missing {item}")
        raise RuntimeError(f"backyard retarget missed {len(missing)}")
    unreal.log(f"imported {total} kit meshes, retargeted {swapped} backyard actors")


main()
