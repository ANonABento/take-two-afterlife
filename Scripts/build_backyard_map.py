# Run inside the UE editor (PythonScriptPlugin), after Scripts/create_toon_assets.py:
#   py Scripts/build_backyard_map.py [downloads folder]   (default: <repo>/Downloads, see Scripts/fetch_assets.ps1)
# The downloads folder holds the unzipped packs from Scripts/backyard_layout.KITS, one sub-folder each named by
# its kit key (KayKitPrototypeBits, KenneyCitySuburban, KenneyNature). Each layout mesh comes from its kit's
# /Game folder first (backyard_layout.kit_root; names matched lowercase, "-" == "_"); only meshes not found
# there are imported from FBX into /Game/Imported/<kit> without materials; a mesh found nowhere becomes a
# toon-coloured /Engine/BasicShapes/Cube placeholder, fitted to the same size, and is listed in
# Saved/backyard_miss_report.txt. Then (re)builds /Game/Maps/Backyard: ground, trench water, route markers,
# dressing, sky and sun, PlayerStart, TTAFinishGate, TTAKillPlane and KillZ. Every slot gets an
# MI_TTA_Toon_Pal_<colour>. Reruns replace everything an earlier run placed (tagged TTA_Layout, plus the
# untagged leftovers of runs from before the tag) and move start, gate and kill plane to the layout's spots.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import backyard_layout as L  # noqa: E402
import layout_spawn as S  # noqa: E402
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
CM = 100.0
PLACEHOLDER = "/Engine/BasicShapes/Cube"
SOLID_ROLES = ("floor", "dressing")              # everything else is seen, not touched
LEGACY_PREFIXES = tuple(f"{role}_" for role in L.ROLES)


def kit_assets(kit):
    """{asset name: asset path} under the kit's /Game folder."""
    paths = EAL.list_assets(L.kit_root(kit), recursive=True, include_folder=False)
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
    """{(kit, mesh): asset path}: the kit folder first, then the FBX import, else the cube; plus the miss list."""
    paths, misses = {}, []
    for kit, names in L.meshes_by_kit().items():
        for name in names:
            imported = f"{L.IMPORT_ROOT}/{kit}/{name}"
            if (kit, name) in found:
                paths[(kit, name)] = assets[kit][found[(kit, name)]]
            elif EAL.does_asset_exist(imported):
                paths[(kit, name)] = imported
                misses.append((kit, name, f"not in {L.kit_root(kit)}, used FBX import {imported}"))
            else:
                paths[(kit, name)] = PLACEHOLDER
                misses.append((kit, name, "not in its kit folder or FBX: placeholder cube"))
                unreal.log_warning(f"{kit}/{name} missing everywhere: placeholder block")
    return paths, misses


def prop_mesh(prop, paths):
    """Kit mesh, else FBX import, else the engine cube (fitted + toon-coloured like the real one)."""
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


def rot(yaw=0.0, pitch=0.0):
    return unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw)


def fit_component(comp, mesh, prop):
    """Child component (the gate Arch): scale + relative offset, bottom-centre on the actor origin."""
    box = mesh.get_bounding_box()
    lo, hi = (box.min.x, box.min.y, box.min.z), (box.max.x, box.max.y, box.max.z)
    scale, rot, _ = S.fit_mesh([h - l for h, l in zip(hi, lo)], target_fit(prop))
    rlo, rhi = S.rotated_box(lo, hi, scale, rot)
    off = S.placement((0.0, 0.0, 0.0), 0.0, rlo, rhi, (1.0, 1.0, 1.0))
    comp.set_relative_rotation(unreal.Rotator(float(rot[0]), float(rot[1]), 0.0), False, False)
    comp.set_relative_scale3d(unreal.Vector(*scale))
    comp.set_relative_rotation(rot(yaw=float(prop.yaw)), False, False)
    comp.set_relative_location(unreal.Vector(*S.placement((0.0, 0.0, 0.0), prop.yaw, lo, hi, scale)), False, False)


def place(actors, prop, toon, paths):
    mesh = prop_mesh(prop, paths)
    extra = (L.ROUTE_TAG,) if prop.role == "marker" else ()
    actor, _ = S.spawn_or_skip(unreal, actors, f"{prop.role}_{prop.mesh}", mesh, target_fit(prop),
                               (prop.x * CM, prop.y * CM, prop.z * CM), prop.yaw, extra)
    if actor is None:   # logged by spawn_or_skip; one bad mesh must not abort the map
        return None
    comp = actor.static_mesh_component
    for slot in range(comp.get_num_materials()):
        comp.set_material(slot, palette_instance(prop.palette, toon))
    return actor


def light_sky(actors):
    """Sun, sky light, atmosphere and thin haze, all dynamic (nothing is baked). Tagged, so a rerun replaces them."""
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000),
                                        rot(yaw=35.0, pitch=-48.0))   # from behind the start, over one shoulder
    sun.set_actor_label("Sun")
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("atmosphere_sun_light", True)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    sky.set_actor_label("SkyLight")
    capture = sky.get_component_by_class(unreal.SkyLightComponent)
    capture.set_mobility(unreal.ComponentMobility.MOVABLE)
    capture.set_editor_property("real_time_capture", True)
    air = actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    air.set_actor_label("SkyAtmosphere")
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("HeightFog")
    haze = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    haze.set_editor_property("fog_density", 0.006)
    haze.set_editor_property("start_distance", 3000.0)
    for actor in (sun, sky, air, fog):
        S.tag(unreal, actor)


def legacy(cls, label):
    """Untagged leftovers of runs from before the TTA_Layout tag: `<role>_<mesh>` meshes and the extra suns."""
    if cls == "StaticMeshActor":
        return label.startswith(LEGACY_PREFIXES)
    return cls == "DirectionalLight" and label == "Sun"


def build(downloads):
    L.validate()
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    assets = {kit: kit_assets(kit) for kit in L.KITS}
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
    # Reruns: what earlier runs placed goes. PlayerStart, gate, kill plane, NavMesh and hand-placed actors stay.
    unreal.log(f"removed {S.delete_previous(actors, legacy)} old {S.LAYOUT_TAG} actors")
    level_actors = actors.get_all_level_actors()

    def first(cls):
        return next((a for a in level_actors if isinstance(a, cls)), None)

    def at(point, lift=0.0):
        return unreal.Vector(point.x * CM, point.y * CM, point.z * CM + lift)

    for prop in L.props():
        if prop.role != "gate":
            place(actors, prop, toon, paths)
            continue
        gate = first(unreal.TTAFinishGate) or actors.spawn_actor_from_class(unreal.TTAFinishGate, at(L.GATE))
        gate.set_actor_location(at(L.GATE), False, False)
        gate.set_actor_rotation(rot(), False)
        mesh = prop_mesh(prop, paths)
        arch = component(gate, "Arch")
        arch.set_static_mesh(mesh)
        fit_component(arch, mesh, prop)
        for slot in range(arch.get_num_materials()):
            arch.set_material(slot, palette_instance(prop.palette, toon))
        hx, hy, hz = L.GATE_TRIGGER_HALF
        component(gate, "Trigger").set_box_extent(unreal.Vector(hx * CM, hy * CM, hz * CM))

    start = first(unreal.PlayerStart) or actors.spawn_actor_from_class(unreal.PlayerStart, at(L.START, 100.0))
    start.set_actor_location(at(L.START, 100.0), False, False)
    start.set_actor_rotation(rot(), False)                       # facing down the route
    k = L.KILL_PLANE
    centre = unreal.Vector((k.min_x + k.max_x) / 2 * CM, (k.min_y + k.max_y) / 2 * CM, (k.min_z + k.max_z) / 2 * CM)
    kill = first(unreal.TTAKillPlane) or actors.spawn_actor_from_class(unreal.TTAKillPlane, centre)
    kill.set_actor_location(centre, False, False)
    component(kill, "Volume").set_box_extent(unreal.Vector((k.max_x - k.min_x) / 2 * CM,
                                                           (k.max_y - k.min_y) / 2 * CM,
                                                           (k.max_z - k.min_z) / 2 * CM))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("kill_z", L.WORLD_KILL_Z * CM)
    # Only the backyard runs the match/build mode (TTABuildHUD); Title overrides to TTATitleGameMode.
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.load_class(None, "/Script/TakeTwoAfterlife.TTAGameMode"))
    light_sky(actors)
    if not level.save_current_level():
        raise RuntimeError(f"could not save {L.MAP_PATH}")
    EAL.save_directory(toon_spec.INSTANCE_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log(f"built {L.MAP_PATH}: {len(L.props())} props")


if len(sys.argv) > 2:
    raise SystemExit("usage: build_backyard_map.py [downloads folder]")
build(L.downloads_dir(sys.argv[1:]))
