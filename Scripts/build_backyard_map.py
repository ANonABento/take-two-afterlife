# Run inside the UE editor (PythonScriptPlugin), after Scripts/create_toon_assets.py:
#   py Scripts/build_backyard_map.py [downloads folder]   (default: <repo>/Downloads, see Scripts/fetch_assets.ps1)
# The downloads folder holds the unzipped packs from Scripts/backyard_layout.KITS, one sub-folder each named by
# its kit key (KayKitPrototypeBits, KenneyCitySuburban, KenneyNature). Each layout mesh comes from
# /Game/ThirdParty/<kit> first (names matched lowercase, "-" == "_"); only meshes not found there are imported
# from FBX into /Game/Imported/<kit> without materials; a mesh found nowhere becomes a toon-coloured
# /Engine/BasicShapes/Cube placeholder, fitted to the same size, and is listed in Saved/backyard_miss_report.txt. Then (re)builds /Game/Maps/Backyard: floor, route markers,
# dressing, PlayerStart, TTAFinishGate, TTAKillPlane and KillZ. Every slot gets an MI_TTA_Toon_Pal_<colour>.
import os
import sys
from pathlib import Path

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import backyard_layout as L  # noqa: E402
import layout_spawn as S  # noqa: E402
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CM = 100.0
PLACEHOLDER = "/Engine/BasicShapes/Cube"


def thirdparty_assets(kit):
    """{asset name: asset path} under /Game/ThirdParty/<kit>."""
    paths = EAL.list_assets(f"{L.THIRDPARTY_ROOT}/{kit}", recursive=True, include_folder=False)
    return {str(p).rsplit("/", 1)[1].split(".", 1)[0]: str(p).split(".", 1)[0] for p in paths}


def import_meshes(downloads, found):
    tasks = []
    for kit, names in L.meshes_by_kit().items():
        for name in names:
            if (kit, name) in found or EAL.does_asset_exist(f"{L.IMPORT_ROOT}/{kit}/{name}"):
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


def resolve_meshes(found, assets):
    """{(kit, mesh): asset path}: ThirdParty first, then the FBX import, else the cube; plus the miss list."""
    paths, misses = {}, []
    for kit, names in L.meshes_by_kit().items():
        for name in names:
            imported = f"{L.IMPORT_ROOT}/{kit}/{name}"
            if (kit, name) in found:
                paths[(kit, name)] = assets[kit][found[(kit, name)]]
            elif EAL.does_asset_exist(imported):
                paths[(kit, name)] = imported
                misses.append((kit, name, f"not in {L.THIRDPARTY_ROOT}, used FBX import {imported}"))
            else:
                paths[(kit, name)] = PLACEHOLDER
                misses.append((kit, name, "not in ThirdParty or FBX: placeholder cube"))
                unreal.log_warning(f"{kit}/{name} missing everywhere: placeholder block")
    return paths, misses


def prop_mesh(prop, paths):
    """ThirdParty mesh, else FBX import, else the engine cube (fitted + toon-coloured like the real one)."""
    return EAL.load_asset(paths[(prop.kit, prop.mesh)])


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


def component(actor, name):
    """VisibleAnywhere components are not Python attributes (they are not BlueprintVisible)."""
    found = actor.get_editor_property(name)
    if found is None:
        raise RuntimeError(f"{actor.get_class().get_name()} has no {name}")
    return found


def target_fit(prop):
    """Layout fit (metres) as a layout_spawn fit (cm)."""
    kind, target = prop.fit
    if kind == "box":
        return ("box", tuple(v * CM for v in target))
    return (kind, target * CM)


def fit_component(comp, mesh, prop):
    """Child component (the gate Arch): scale + relative offset, bottom-centre on the actor origin."""
    box = mesh.get_bounding_box()
    lo, hi = (box.min.x, box.min.y, box.min.z), (box.max.x, box.max.y, box.max.z)
    scale = S.fit_scale([h - l for h, l in zip(hi, lo)], target_fit(prop))
    off = S.placement((0.0, 0.0, 0.0), 0.0, lo, hi, scale)
    comp.set_relative_scale3d(unreal.Vector(*scale))
    comp.set_relative_location(unreal.Vector(*off), False, False)


def place(actors, prop, toon, paths):
    mesh = prop_mesh(prop, paths)
    extra = (L.ROUTE_TAG,) if prop.role == "marker" else ()
    actor, _ = S.spawn_fitted(unreal, actors, f"{prop.role}_{prop.mesh}", mesh, target_fit(prop),
                              (prop.x * CM, prop.y * CM, prop.z * CM), prop.yaw, extra)
    comp = actor.static_mesh_component
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
    assets = {kit: thirdparty_assets(kit) for kit in L.KITS}
    found, _ = L.resolve_all({kit: list(a) for kit, a in assets.items()})
    import_meshes(downloads, found)
    paths, misses = resolve_meshes(found, assets)
    L.MISS_REPORT.parent.mkdir(parents=True, exist_ok=True)
    L.MISS_REPORT.write_text(L.miss_report(misses), encoding="utf-8")
    unreal.log(f"{len(misses)} mesh misses, see {L.MISS_REPORT}")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(L.MAP_PATH):
        level.load_level(L.MAP_PATH)
    elif not level.new_level(L.MAP_PATH):
        raise RuntimeError(f"could not create {L.MAP_PATH}")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # Reruns: only what an earlier run tagged TTA_Layout goes. PlayerStart, gate, kill plane, KillZ, NavMesh stay.
    unreal.log(f"removed {S.delete_previous(actors)} old {S.LAYOUT_TAG} actors")
    level_actors = actors.get_all_level_actors()

    def first(cls):
        return next((a for a in level_actors if isinstance(a, cls)), None)

    for prop in L.props():
        if prop.role != "gate":
            place(actors, prop, toon, paths)
        elif first(unreal.TTAFinishGate) is None:
            gate = actors.spawn_actor_from_class(unreal.TTAFinishGate, unreal.Vector(L.GATE.x * CM, L.GATE.y * CM,
                                                                                     L.GATE.z * CM))
            mesh = prop_mesh(prop, paths)
            arch = component(gate, "Arch")
            arch.set_static_mesh(mesh)
            fit_component(arch, mesh, prop)
            for slot in range(arch.get_num_materials()):
                arch.set_material(slot, palette_instance(prop.palette, toon))
            hx, hy, hz = L.GATE_TRIGGER_HALF
            component(gate, "Trigger").set_box_extent(unreal.Vector(hx * CM, hy * CM, hz * CM))

    if first(unreal.PlayerStart) is None:
        actors.spawn_actor_from_class(unreal.PlayerStart,
                                      unreal.Vector(L.START.x * CM, L.START.y * CM, L.START.z * CM + 100.0))
    first_build = first(unreal.TTAKillPlane) is None
    if first_build:
        k = L.KILL_PLANE
        kill = actors.spawn_actor_from_class(unreal.TTAKillPlane, unreal.Vector(
            (k.min_x + k.max_x) / 2 * CM, (k.min_y + k.max_y) / 2 * CM, (k.min_z + k.max_z) / 2 * CM))
        component(kill, "Volume").set_box_extent(unreal.Vector((k.max_x - k.min_x) / 2 * CM,
                                                               (k.max_y - k.min_y) / 2 * CM,
                                                               (k.max_z - k.min_z) / 2 * CM))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if first_build:
        world.get_world_settings().set_editor_property("kill_z", L.WORLD_KILL_Z * CM)
    # Only the backyard runs the match/build mode (TTABuildHUD); Title overrides to TTATitleGameMode.
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.load_class(None, "/Script/TakeTwoAfterlife.TTAGameMode"))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                        unreal.Rotator(0.0, -50.0, 30.0))
    sun.set_actor_label("Sun")
    S.tag(unreal, sun)
    if not level.save_current_level():
        raise RuntimeError(f"could not save {L.MAP_PATH}")
    EAL.save_directory(toon_spec.INSTANCE_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log(f"built {L.MAP_PATH}: {len(L.props())} props")


if len(sys.argv) > 2:
    raise SystemExit("usage: build_backyard_map.py [downloads folder]")
build(L.downloads_dir(sys.argv[1:]))
