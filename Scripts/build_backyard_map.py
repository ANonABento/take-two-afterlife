# Run inside the UE editor (PythonScriptPlugin), after Scripts/create_toon_assets.py:
#   py Scripts/build_backyard_map.py [downloads folder]   (default: <repo>/Downloads, see Scripts/fetch_assets.ps1)
# The downloads folder holds the unzipped packs from Scripts/backyard_layout.KITS, one sub-folder each named by
# its kit key (KayKitPrototypeBits, KenneyCitySuburban, KenneyNature). Imports the FBX meshes the layout uses
# into /Game/Imported/<kit> without their materials (a missing pack or mesh becomes a toon-coloured
# /Engine/BasicShapes/Cube placeholder, fitted to the same size), then (re)builds /Game/Maps/Backyard: floor, route markers,
# dressing, PlayerStart, TTAFinishGate, TTAKillPlane and KillZ. Every slot gets an MI_TTA_Toon_Pal_<colour>.
import os
import sys
from pathlib import Path

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import backyard_layout as L  # noqa: E402
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CM = 100.0
PLACEHOLDER = "/Engine/BasicShapes/Cube"


def import_meshes(downloads):
    tasks = []
    for kit, names in L.meshes_by_kit().items():
        for name in names:
            if EAL.does_asset_exist(f"{L.IMPORT_ROOT}/{kit}/{name}"):
                continue
            fbx = L.find_fbx(downloads, kit, name)
            if fbx is None:
                unreal.log_warning(f"{kit}/{name}.fbx not in {downloads}: using a placeholder block ({L.KITS[kit].url})")
                continue
            ui = unreal.FbxImportUI()
            ui.import_materials = False
            ui.import_textures = False
            ui.import_as_skeletal = False
            ui.static_mesh_import_data.combine_meshes = True
            task = unreal.AssetImportTask()
            task.filename = str(fbx)
            task.destination_path = f"{L.IMPORT_ROOT}/{kit}"
            task.destination_name = name
            task.automated = True
            task.save = True
            task.options = ui
            tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def prop_mesh(prop):
    """Imported kit mesh, or the engine cube (fitted + toon-coloured like the real one) when it is missing."""
    path = f"{L.IMPORT_ROOT}/{prop.kit}/{prop.mesh}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    unreal.log_warning(f"{path} missing: placeholder block")
    return EAL.load_asset(PLACEHOLDER)


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


def fit(component, mesh, prop):
    """Scale + offset so the mesh's bounding box bottom-centre lands on the prop position at the fitted size."""
    box = mesh.get_bounding_box()
    size = box.max - box.min
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


def place(actors, prop, toon):
    mesh = prop_mesh(prop)
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(prop.x * CM, prop.y * CM, prop.z * CM),
                                          unreal.Rotator(0.0, 0.0, float(prop.yaw)))
    actor.set_actor_label(f"{prop.role}_{prop.mesh}")
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    fit(comp, mesh, prop)
    for slot in range(comp.get_num_materials()):
        comp.set_material(slot, palette_instance(prop.palette, toon))
    if prop.role == "marker":
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


def build(downloads):
    L.validate()
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    import_meshes(downloads)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(L.MAP_PATH):
        level.load_level(L.MAP_PATH)
    elif not level.new_level(L.MAP_PATH):
        raise RuntimeError(f"could not create {L.MAP_PATH}")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, (unreal.StaticMeshActor, unreal.PlayerStart, unreal.TTAFinishGate, unreal.TTAKillPlane)):
            actors.destroy_actor(actor)

    for prop in L.props():
        if prop.role == "gate":
            gate = actors.spawn_actor_from_class(unreal.TTAFinishGate, unreal.Vector(L.GATE.x * CM, L.GATE.y * CM,
                                                                                     L.GATE.z * CM))
            mesh = prop_mesh(prop)
            gate.arch.set_static_mesh(mesh)
            fit(gate.arch, mesh, prop)
            for slot in range(gate.arch.get_num_materials()):
                gate.arch.set_material(slot, palette_instance(prop.palette, toon))
            hx, hy, hz = L.GATE_TRIGGER_HALF
            gate.trigger.set_box_extent(unreal.Vector(hx * CM, hy * CM, hz * CM))
        else:
            place(actors, prop, toon)

    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(L.START.x * CM, L.START.y * CM, L.START.z * CM + 100.0))
    k = L.KILL_PLANE
    kill = actors.spawn_actor_from_class(unreal.TTAKillPlane, unreal.Vector(
        (k.min_x + k.max_x) / 2 * CM, (k.min_y + k.max_y) / 2 * CM, (k.min_z + k.max_z) / 2 * CM))
    kill.volume.set_box_extent(unreal.Vector((k.max_x - k.min_x) / 2 * CM, (k.max_y - k.min_y) / 2 * CM,
                                             (k.max_z - k.min_z) / 2 * CM))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("kill_z", L.WORLD_KILL_Z * CM)
    # Only the backyard runs the match/build mode (TTABuildHUD); Title overrides to TTATitleGameMode.
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.load_class(None, "/Script/TakeTwoAfterlife.TTAGameMode"))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                        unreal.Rotator(0.0, -50.0, 30.0))
    sun.set_actor_label("Sun")
    if not level.save_current_level():
        raise RuntimeError(f"could not save {L.MAP_PATH}")
    EAL.save_directory(toon_spec.INSTANCE_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log(f"built {L.MAP_PATH}: {len(L.props())} props")


if len(sys.argv) > 2:
    raise SystemExit("usage: build_backyard_map.py [downloads folder]")
build(L.downloads_dir(sys.argv[1:]))
