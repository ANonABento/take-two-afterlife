# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/dress_heaven.py -unattended -nosplash
# Dresses /Game/Maps/Heaven: soft pastel sky and fog, the sun feeds the atmosphere, four
# player starts in a row facing the screen, toon clouds, and the podium set. ATTACinema
# does not look up podium spots, so 1st/2nd/3rd are TargetPoints tagged 1st, 2nd and 3rd.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MAP_PATH = "/Game/Maps/Heaven"
# Cinema actor space (cm): couch at the origin, screen at +X. Matches TTACinema.cpp.
SCREEN = unreal.Vector(1200.0, 0.0, 350.0)
SEAT = unreal.Vector(0.0, 0.0, 45.0)
START_X = -300.0
START_YS = (-135.0, -45.0, 45.0, 135.0)
PODIUM_X = 780.0
COLOUR = {
    "SM_Heaven_Cloud_A": "Bone",
    "SM_Heaven_Cloud_B": "Sky",
    "SM_Heaven_Cloud_C": "Bone",
    "SM_Heaven_Podium": "Wood",
    "SM_Heaven_FilmReelTrophy": "Yellow",
    "SM_Heaven_Halo": "Yellow",
    "SM_Heaven_RedCarpet": "Red",
    "SM_Heaven_PearlyGate": "Bone",
    "SM_Heaven_VelvetRopeStanchions": "Yellow",
}


def palette_instance(colour, toon):
    path = f"{toon_spec.INSTANCE_DIR}/MI_TTA_Toon_Pal_{colour}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        path.rsplit("/", 1)[1], toon_spec.INSTANCE_DIR, unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, toon)
    MEL.set_material_instance_scalar_parameter_value(mi, "UsePaletteUV", 0.0)
    index = [name for name, _ in toon_spec.PALETTE].index(colour)
    MEL.set_material_instance_scalar_parameter_value(mi, "PaletteIndex", float(index))
    EAL.save_loaded_asset(mi)
    return mi


def load_mesh(name):
    for folder in ("/Game/Art/Heaven", "/Game/Art/Props"):
        path = f"{folder}/{name}"
        if EAL.does_asset_exist(path):
            return EAL.load_asset(path)
    raise RuntimeError(f"missing mesh {name}")


def sensible_scale(mesh):
    """Imported centimetre meshes stay at 1. A metres-sized import (a podium under 20 cm) is scaled up."""
    box = mesh.get_bounding_box()
    height = box.max.z - box.min.z
    if height < 20.0:
        return unreal.Vector(100.0, 100.0, 100.0)
    return unreal.Vector(1.0, 1.0, 1.0)


def ground_location(mesh, x, y, scale, ground_z=0.0):
    box = mesh.get_bounding_box()
    return unreal.Vector(x, y, ground_z - box.min.z * scale.z)


def paint(comp, colour, toon):
    material = palette_instance(colour, toon)
    for slot in range(max(comp.get_num_materials(), 1)):
        comp.set_material(slot, material)


def by_label(actors, label):
    return next((actor for actor in actors if actor.get_actor_label() == label), None)


def spawn_mesh(subsystem, existing, label, mesh, location, rotation, colour, toon, scale):
    actor = existing.get(label)
    if actor is None:
        actor = subsystem.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
        actor.set_actor_label(label)
    else:
        actor.set_actor_location(location, False, False)
        actor.set_actor_rotation(rotation, False)
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_relative_scale3d(scale)
    paint(comp, colour, toon)
    return actor


def main():
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level.load_level(MAP_PATH):
        raise RuntimeError(f"could not load {MAP_PATH}")
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = subsystem.get_all_level_actors()
    existing = {actor.get_actor_label(): actor for actor in actors}

    if by_label(actors, "Sky") is None or not any(isinstance(a, unreal.SkyAtmosphere) for a in actors):
        sky = subsystem.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
        sky.set_actor_label("SkyAtmosphere")

    fog = next((a for a in actors if isinstance(a, unreal.ExponentialHeightFog)), None)
    if fog is None:
        fog = subsystem.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
        fog.set_actor_label("Fog")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    pastel = unreal.LinearColor(0.72, 0.62, 0.86, 1.0)
    fog_comp.set_editor_property("fog_density", 0.012)
    fog_comp.set_editor_property("fog_height_falloff", 0.18)
    fog_comp.set_editor_property("fog_max_opacity", 0.35)
    fog_comp.set_editor_property("start_distance", 800.0)
    for prop in ("fog_inscattering_luminance", "fog_inscattering_color"):
        try:
            fog_comp.set_editor_property(prop, pastel)
            break
        except Exception:
            continue

    sun = next((a for a in actors if isinstance(a, unreal.DirectionalLight)), None)
    if sun is None:
        sun = subsystem.spawn_actor_from_class(
            unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-40.0, 30.0, 0.0))
        sun.set_actor_label("Sun")
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_editor_property("atmosphere_sun_light", True)
    light.set_editor_property("atmosphere_sun_light_index", 0)

    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    facing = unreal.Rotator(0.0, 0.0, 0.0)
    for index, y in enumerate(START_YS):
        loc = unreal.Vector(START_X, y, 100.0)
        if index < len(starts):
            start = starts[index]
        else:
            start = subsystem.spawn_actor_from_class(unreal.PlayerStart, loc, facing)
        start.set_actor_label(f"PlayerStart_{index + 1}")
        start.set_actor_location(loc, False, False)
        start.set_actor_rotation(facing, False)

    cinema = next((a for a in actors if isinstance(a, unreal.TTACinema)), None)
    if cinema is None:
        raise RuntimeError("Heaven has no ATTACinema")
    origin = cinema.get_actor_transform()
    couch = cinema.get_editor_property("couch")
    screen = cinema.get_editor_property("screen")
    cloud = cinema.get_editor_property("cloud")
    couch.set_static_mesh(load_mesh("SM_Heaven_Couch"))
    screen.set_static_mesh(load_mesh("SM_Heaven_Screen"))
    cloud.set_static_mesh(load_mesh("SM_Heaven_Cloud"))
    # The cinema C++ defaults scale the engine cube/cylinder. The imported meshes are already in centimetres.
    for comp in (cloud, couch, screen):
        comp.set_relative_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    couch.set_world_location(origin.transform_location(SEAT), False, False)
    screen.set_world_location(origin.transform_location(SCREEN), False, False)
    for comp, colour in ((cloud, "Bone"), (couch, "Pink"), (screen, "Ink")):
        paint(comp, colour, toon)

    cloud_spots = (("SM_Heaven_Cloud_A", -600.0, -1400.0, 280.0),
                   ("SM_Heaven_Cloud_B", 1800.0, 1100.0, 420.0),
                   ("SM_Heaven_Cloud_C", 1600.0, -1600.0, 180.0))
    for name, x, y, z in cloud_spots:
        mesh = load_mesh(name)
        scale = sensible_scale(mesh)
        loc = unreal.Vector(x, y, z - mesh.get_bounding_box().min.z * scale.z)
        spawn_mesh(subsystem, existing, name, mesh, loc, unreal.Rotator(0, 0, 0), COLOUR[name], toon, scale)

    podium_mesh = load_mesh("SM_Heaven_Podium")
    podium_scale = sensible_scale(podium_mesh)
    podium_loc = ground_location(podium_mesh, PODIUM_X, 0.0, podium_scale)
    podium = spawn_mesh(subsystem, existing, "Podium", podium_mesh, podium_loc,
                        unreal.Rotator(0, 180.0, 0), COLOUR["SM_Heaven_Podium"], toon, podium_scale)
    podium_box = podium_mesh.get_bounding_box()
    top_z = podium_loc.z + podium_box.max.z * podium_scale.z
    width = (podium_box.max.y - podium_box.min.y) * podium_scale.y

    trophy_mesh = load_mesh("SM_Heaven_FilmReelTrophy")
    trophy_scale = sensible_scale(trophy_mesh)
    trophy_loc = unreal.Vector(PODIUM_X, 0.0, top_z - trophy_mesh.get_bounding_box().min.z * trophy_scale.z)
    trophy = spawn_mesh(subsystem, existing, "Trophy", trophy_mesh, trophy_loc, unreal.Rotator(0, 180.0, 0),
                        COLOUR["SM_Heaven_FilmReelTrophy"], toon, trophy_scale)
    trophy_top = trophy_loc.z + trophy_mesh.get_bounding_box().max.z * trophy_scale.z

    halo_mesh = load_mesh("SM_Heaven_Halo")
    halo_scale = sensible_scale(halo_mesh)
    halo_loc = unreal.Vector(PODIUM_X, 0.0, trophy_top + 25.0 - halo_mesh.get_bounding_box().min.z * halo_scale.z)
    spawn_mesh(subsystem, existing, "Halo", halo_mesh, halo_loc, unreal.Rotator(0, 0, 0),
               COLOUR["SM_Heaven_Halo"], toon, halo_scale)

    carpet_mesh = load_mesh("SM_Heaven_RedCarpet")
    carpet_scale = sensible_scale(carpet_mesh)
    carpet_box = carpet_mesh.get_bounding_box()
    carpet_length = (carpet_box.max.x - carpet_box.min.x) * carpet_scale.x
    # Lay the carpet from the couch toward the podium, sitting on the cloud.
    carpet_x = PODIUM_X - (podium_box.max.x - podium_box.min.x) * podium_scale.x * 0.5 - carpet_length * 0.5
    carpet_loc = ground_location(carpet_mesh, carpet_x, 0.0, carpet_scale)
    spawn_mesh(subsystem, existing, "RedCarpet", carpet_mesh, carpet_loc, unreal.Rotator(0, 0, 0),
               COLOUR["SM_Heaven_RedCarpet"], toon, carpet_scale)

    # No ATTACinema lookup for podium marks. Tags 1st/2nd/3rd are the spots.
    spots = (("Podium_1st", "1st", 0.0, 12.0),
             ("Podium_2nd", "2nd", -width * 0.28, 0.0),
             ("Podium_3rd", "3rd", width * 0.28, 0.0))
    fresh = {actor.get_actor_label(): actor for actor in subsystem.get_all_level_actors()}
    for label, tag, y, lift in spots:
        loc = unreal.Vector(PODIUM_X, y, top_z + lift)
        point = fresh.get(label)
        if point is None:
            point = subsystem.spawn_actor_from_class(unreal.TargetPoint, loc, unreal.Rotator(0, 180.0, 0))
            point.set_actor_label(label)
        else:
            point.set_actor_location(loc, False, False)
            point.set_actor_rotation(unreal.Rotator(0, 180.0, 0), False)
        point.set_editor_property("tags", [unreal.Name(tag)])

    if not level.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log("dressed /Game/Maps/Heaven")


main()
