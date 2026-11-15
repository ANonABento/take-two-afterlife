# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/dress_heaven.py -unattended -nosplash
# Dresses /Game/Maps/Heaven: soft pastel sky and fog, the sun feeds the atmosphere, four
# player starts in a row facing the screen, toon clouds, and the podium set. ATTACinema
# does not look up podium spots, so 1st/2nd/3rd are TargetPoints tagged 1st, 2nd and 3rd.
import json
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import layout_spawn as S  # noqa: E402
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
LAYOUT_JSON = os.path.join(os.path.dirname(os.path.abspath(__file__)), "heaven_layout.json")
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


def paint(comp, colour, toon):
    material = palette_instance(colour, toon)
    for slot in range(max(comp.get_num_materials(), 1)):
        comp.set_material(slot, material)


def by_label(actors, label):
    return next((actor for actor in actors if actor.get_actor_label() == label), None)


def spawn_mesh(subsystem, piece, toon):
    """piece: one S.heaven_pieces() entry. Fitted once from the mesh bounds, tagged TTA_Layout."""
    mesh = load_mesh(piece["mesh"])
    actor, size = S.spawn_or_skip(unreal, subsystem, piece["label"], mesh, piece["fit"], piece["at"], piece["yaw"])
    if actor is None:   # logged by spawn_or_skip
        return None, 0.0, 0.0
    paint(actor.static_mesh_component, piece["colour"], toon)
    return actor, size[2], size[1]


def main():
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level.load_level(MAP_PATH):
        raise RuntimeError(f"could not load {MAP_PATH}")
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # Reruns: delete what the last run spawned (TTA_Layout). PlayerStarts, cinema, sky, fog, sun stay.
    unreal.log(f"removed {S.delete_previous(subsystem)} old {S.LAYOUT_TAG} actors")
    actors = subsystem.get_all_level_actors()

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

    with open(LAYOUT_JSON, encoding="utf-8") as f:
        layout = json.load(f)
    pieces = {p["label"]: p for p in S.heaven_pieces(layout)}
    for piece in pieces.values():
        if piece["label"] not in ("Podium", "Trophy", "Halo"):
            spawn_mesh(subsystem, piece, toon)

    # Lobby gate trigger on the red carpet: in Lobby, going through it marks a player ready.
    gate_at = unreal.Vector(*next(p for p in pieces.values() if p["mesh"] == "SM_Heaven_RedCarpet")["at"])
    gate = next((a for a in actors if isinstance(a, unreal.TTAFinishGate)), None)
    if gate is None:
        gate = subsystem.spawn_actor_from_class(unreal.TTAFinishGate, gate_at)
    gate.set_actor_label("HeavenGate")
    gate.set_actor_location(gate_at, False, False)

    podium, podium_h, width = spawn_mesh(subsystem, pieces["Podium"], toon)
    top_z = pieces["Podium"]["at"][2] + podium_h
    trophy = dict(pieces["Trophy"], at=(PODIUM_X, 0.0, top_z))
    _, trophy_h, _ = spawn_mesh(subsystem, trophy, toon)
    spawn_mesh(subsystem, dict(pieces["Halo"], at=(PODIUM_X, 0.0, top_z + trophy_h + 25.0)), toon)

    # No ATTACinema lookup for podium marks. Tags 1st/2nd/3rd are the spots.
    spots = (("Podium_1st", "1st", 0.0, 12.0),
             ("Podium_2nd", "2nd", -width * 0.28, 0.0),
             ("Podium_3rd", "3rd", width * 0.28, 0.0))
    for label, tag, y, lift in spots:
        point = subsystem.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(PODIUM_X, y, top_z + lift),
                                                 unreal.Rotator(0, 180.0, 0))
        point.set_actor_label(label)
        S.tag(unreal, point, tag)

    if not level.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log("dressed /Game/Maps/Heaven")


main()
