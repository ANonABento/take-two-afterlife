# Run inside the UE editor (PythonScriptPlugin), after Scripts/create_toon_assets.py:
#   py Scripts/build_heaven_map.py <folder with SM_Heaven_*.fbx>   (default: <project>/models)
# Imports the cloud, 4-seat couch and screen meshes (models/build_heaven.py) into /Game/Art/Heaven without
# materials, then (re)builds /Game/Maps/Heaven: one ATTACinema at the origin using those meshes, every slot
# an MI_TTA_Toon_Pal_<colour> instance (TTA.Art.AllToon), PlayerStart behind the couch, sun + sky light.
import os
import sys
from pathlib import Path

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Scripts"))  # toon_spec lives in Scripts/
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAP_PATH = "/Game/Maps/Heaven"
IMPORT_DIR = "/Game/Art/Heaven"
MESHES = ("SM_Heaven_Cloud", "SM_Heaven_Couch", "SM_Heaven_Screen")
# Blender material name (= UE slot name) -> toon palette colour.
SLOT_COLOUR = {"Cloud": "Bone", "CloudShade": "Sky", "Couch": "Pink", "Cushion": "Bone",
               "Gold": "Yellow", "Screen": "Ink"}


def palette_instance(colour, toon):
    """Same MI_TTA_Toon_Pal_<colour> instances as Scripts/build_backyard_map.py (that module runs on import)."""
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
    index = [n for n, _ in toon_spec.PALETTE].index(colour)
    MEL.set_material_instance_scalar_parameter_value(mi, "PaletteIndex", float(index))
    EAL.save_loaded_asset(mi)
    return mi


def import_meshes(folder):
    tasks = []
    for name in MESHES:
        fbx = Path(folder) / f"{name}.fbx"
        if not fbx.is_file():
            raise RuntimeError(f"missing {fbx}: run models/build_heaven.py in Blender")
        ui = unreal.FbxImportUI()
        ui.import_materials = False
        ui.import_textures = False
        ui.import_as_skeletal = False
        ui.static_mesh_import_data.combine_meshes = True
        task = unreal.AssetImportTask()
        task.filename = str(fbx)
        task.destination_path = IMPORT_DIR
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        task.options = ui
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    meshes = {}
    for name in MESHES:
        mesh = EAL.load_asset(f"{IMPORT_DIR}/{name}")
        if mesh is None:
            raise RuntimeError(f"import failed for {name}")
        meshes[name] = mesh
    return meshes


def skin(comp, mesh, toon):
    comp.set_static_mesh(mesh)
    comp.set_relative_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    for slot, slot_name in enumerate(comp.get_material_slot_names()):
        colour = SLOT_COLOUR.get(str(slot_name).split("_")[0], "Bone")
        comp.set_material(slot, palette_instance(colour, toon))


def build(folder):
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    meshes = import_meshes(folder)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(MAP_PATH):
        level.load_level(MAP_PATH)
    elif not level.new_level(MAP_PATH):
        raise RuntimeError(f"could not create {MAP_PATH}")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, (unreal.TTACinema, unreal.StaticMeshActor, unreal.PlayerStart,
                              unreal.DirectionalLight, unreal.SkyLight)):
            actors.destroy_actor(actor)

    cinema = actors.spawn_actor_from_class(unreal.TTACinema, unreal.Vector(0, 0, 0))
    cinema.set_actor_label("Cinema")
    skin(cinema.get_editor_property("cloud"), meshes["SM_Heaven_Cloud"], toon)
    skin(cinema.get_editor_property("couch"), meshes["SM_Heaven_Couch"], toon)
    skin(cinema.get_editor_property("screen"), meshes["SM_Heaven_Screen"], toon)

    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-300.0, 0.0, 100.0))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                        unreal.Rotator(0.0, -40.0, 30.0))
    sun.set_actor_label("Sun")
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500)).set_actor_label("Sky")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("kill_z", -2000.0)
    if not level.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    EAL.save_directory(toon_spec.INSTANCE_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log(f"built {MAP_PATH}")


default = Path(__file__).resolve().parents[1] / "models"
build(sys.argv[1] if len(sys.argv) > 1 else str(default))
