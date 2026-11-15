# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/dress_heaven.py -unattended -nosplash
# Dresses /Game/Maps/Heaven from Scripts/heaven_layout.json: soft pastel sky and fog, the sun feeds the
# atmosphere, four player starts behind the couch, the pearly lobby gate and its carpet on the +Y side, the
# podium set on the -Y side (nothing between the couch and the screen). ATTACinema does not look up podium
# spots, so 1st/2nd/3rd are TargetPoints tagged 1st, 2nd and 3rd. Reruns replace everything it spawned,
# including untagged leftovers of runs from before the TTA_Layout tag.
import json
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hand_dressed  # noqa: E402
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
LAYOUT_JSON = os.path.join(os.path.dirname(os.path.abspath(__file__)), "heaven_layout.json")
GATE_COLOUR = "Bone"


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


def cinema_point(origin, at):
    """Layout spot (cinema space, z above the cloud top) -> world cm. The cloud floor is the cinema's root, so
    the floor top is wherever the cinema actor stands, not world Z 0."""
    p = origin.transform_location(unreal.Vector(at[0], at[1], at[2] + S.FLOOR_TOP))
    return (p.x, p.y, p.z)


def spawn_mesh(subsystem, piece, toon, origin):
    """piece: one S.heaven_pieces() entry. Fitted once from the mesh bounds, bounds bottom-centre on its spot."""
    mesh = load_mesh(piece["mesh"])
    yaw = piece["yaw"] + origin.rotation.yaw
    actor, size = S.spawn_or_skip(unreal, subsystem, piece["label"], mesh, piece["fit"], cinema_point(origin, piece["at"]),
                                  yaw, collide=piece["collide"])
    if actor is None:   # logged by spawn_or_skip
        return None, 0.0, 0.0
    paint(actor.static_mesh_component, piece["colour"], toon)
    return actor, size[2], size[1]


def main():
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing")
    with open(LAYOUT_JSON, encoding="utf-8") as f:
        layout = json.load(f)
    pieces = S.heaven_pieces(layout)
    if S.mesh_bounds(load_mesh("SM_Heaven_Podium"))[1][2] != S.PODIUM_SIZE[2]:
        raise RuntimeError("SM_Heaven_Podium changed: update layout_spawn.PODIUM_STEPS")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level.load_level(MAP_PATH):
        raise RuntimeError(f"could not load {MAP_PATH}")
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world_settings = world.get_world_settings() if world else None
    if hand_dressed.is_hand_dressed(subsystem.get_all_level_actors(), world_settings):
        unreal.log_warning(f"{MAP_PATH} is tagged {hand_dressed.HAND_DRESSED_TAG}: dress_heaven.py skipped, map left as is")
        return
    # Reruns: delete what the last run spawned (TTA_Layout). PlayerStarts, cinema, sky, fog, sun stay.
    unreal.log(f"removed {S.delete_previous(subsystem)} old {S.LAYOUT_TAG} actors")
    legacy = S.heaven_labels(layout)
    unreal.log(f"removed {S.delete_previous(subsystem, lambda cls, label: cls in ('StaticMeshActor', 'TargetPoint') and label in legacy)} untagged legacy actors")
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
        sun = subsystem.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000))
        sun.set_actor_label("Sun")
    # From behind the couch on the gate side: lights the screen, the podium plates and the gate front.
    sun.set_actor_rotation(rot(yaw=-40.0, pitch=-45.0), False)
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_editor_property("atmosphere_sun_light", True)
    light.set_editor_property("atmosphere_sun_light_index", 0)

    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    for index, y in enumerate(START_YS):
        loc = unreal.Vector(START_X, y, 100.0)
        if index < len(starts):
            start = starts[index]
        else:
            start = subsystem.spawn_actor_from_class(unreal.PlayerStart, loc, rot())
        start.set_actor_label(f"PlayerStart_{index + 1}")
        start.set_actor_location(loc, False, False)
        start.set_actor_rotation(rot(), False)

    cinema = next((a for a in actors if isinstance(a, unreal.TTACinema)), None)
    if cinema is None:
        raise RuntimeError("Heaven has no ATTACinema")
    origin = cinema.get_actor_transform()
    couch = cinema.get_editor_property("couch")
    screen = cinema.get_editor_property("screen")
    cloud = cinema.get_editor_property("cloud")
    floor = load_mesh("SM_Heaven_Cloud")
    walkable_floor(floor)
    couch.set_static_mesh(load_mesh("SM_Heaven_Couch"))
    screen.set_static_mesh(load_mesh("SM_Heaven_Screen"))
    cloud.set_static_mesh(floor)
    # The cinema C++ defaults scale the engine cube/cylinder. The imported meshes are already in centimetres.
    for comp in (cloud, couch, screen):
        comp.set_relative_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    couch.set_world_location(origin.transform_location(SEAT), False, False)
    screen.set_world_location(origin.transform_location(SCREEN), False, False)
    for comp, colour in ((cloud, "Bone"), (couch, "Pink"), (screen, "Ink")):
        paint(comp, colour, toon)

    for piece in pieces:
        spawn_mesh(subsystem, piece, toon, origin)

    # Lobby gate: the pearly arch at the end of the red carpet. In Lobby, going through it marks a player
    # ready. The trigger is thin along the gate actor's X, so the actor is turned to face down the carpet.
    spot = S.heaven_protected(layout)["Gate"]
    gate_at = unreal.Vector(*cinema_point(origin, (spot["x"], spot["y"], 0.0)))
    gate = next((a for a in actors if isinstance(a, unreal.TTAFinishGate)), None)
    if gate is None:
        gate = subsystem.spawn_actor_from_class(unreal.TTAFinishGate, gate_at)
    gate.set_actor_label("HeavenGate")
    gate.set_actor_location(gate_at, False, False)
    gate.set_actor_rotation(rot(yaw=spot["yaw"] + 90.0), False)
    arch_mesh = load_mesh("SM_Heaven_PearlyGate")
    arch = gate.get_editor_property("arch")
    arch.set_static_mesh(arch_mesh)
    lo, hi = S.mesh_bounds(arch_mesh)
    scale = S.fit_scale([h - l for h, l in zip(hi, lo)], S.HEAVEN_FIT["SM_Heaven_PearlyGate"])
    arch.set_relative_scale3d(unreal.Vector(*scale))
    arch.set_relative_rotation(rot(yaw=-90.0), False, False)
    arch.set_relative_location(unreal.Vector(*S.placement((0.0, 0.0, 0.0), -90.0, lo, hi, scale)), False, False)
    paint(arch, GATE_COLOUR, toon)
    trigger = gate.get_editor_property("trigger")
    trigger.set_box_extent(unreal.Vector(*S.GATE_TRIGGER_HALF))
    trigger.set_relative_location(unreal.Vector(0.0, 0.0, S.GATE_TRIGGER_HALF[2]), False, False)

    # No ATTACinema lookup for podium marks. Tags 1st/2nd/3rd are the spots (statue centre, facing the lobby).
    for tag, (x, y, z, yaw) in S.podium_spots(layout).items():
        point = subsystem.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(*cinema_point(origin, (x, y, z))),
                                                 rot(yaw=yaw))
        point.set_actor_label(f"Podium_{tag}")
        S.tag(unreal, point, tag)

    if not level.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log("dressed /Game/Maps/Heaven")


main()


def walkable_floor(mesh):
    """Collide with the cloud's triangles so runners stand on the cloud they see."""
    body = mesh.get_editor_property("body_setup")
    flag = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    if body.get_editor_property("collision_trace_flag") != flag:
        body.set_editor_property("collision_trace_flag", flag)
        EAL.save_loaded_asset(mesh)


def rot(yaw=0.0, pitch=0.0):
    return unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw)
