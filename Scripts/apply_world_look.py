# Run inside the UE editor (PythonScriptPlugin):
#   py Scripts/apply_world_look.py
# For each map in toon_spec.LOOK_MAPS: removes earlier look volumes (tag TTAWorldLook), places one unbound
# PostProcessVolume with M_TTA_PP_Outline as a blendable, tagged with its preset name, and saves the map.
# Then writes the default preset (DEFAULT_PRESET) into the MPC_TTA_World asset's parameter defaults and saves
# the asset, so packaged builds keep it. UTTAWorldLookLibrary::ApplyWorldLook switches presets at runtime.
# UE 5.5 Python has no KismetMaterialLibrary / EditorLevelLibrary; this uses asset defaults + editor subsystems.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary
# One asset holds one set of defaults: Course (Title + Backyard), the same values create_toon_assets.py uses.
DEFAULT_PRESET = "Course"


def load(path):
    asset = EAL.load_asset(path)
    if asset is None:
        raise RuntimeError(f"{path} is missing; run create_toon_assets.py first")
    return asset


def place_volume(actors, outline, preset):
    for actor in actors.get_all_level_actors():
        if actor.actor_has_tag(toon_spec.LOOK_TAG):
            actors.destroy_actor(actor)
    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_editor_property("unbound", True)
    volume.set_actor_label(f"{toon_spec.LOOK_TAG}_{preset}")
    volume.set_editor_property("tags", [toon_spec.LOOK_TAG, preset])
    blendable = unreal.WeightedBlendable()
    blendable.set_editor_property("weight", 1.0)
    blendable.set_editor_property("object", outline)
    blendables = unreal.WeightedBlendables()
    blendables.set_editor_property("array", [blendable])
    settings = volume.get_editor_property("settings")
    settings.set_editor_property("weighted_blendables", blendables)
    volume.set_editor_property("settings", settings)


def write_mpc_defaults(mpc, presets, preset="Course"):
    """Set MPC_TTA_World scalar (and vector, if a preset has any) defaults from a preset and save the asset."""
    values = dict(presets[preset])
    for prop in ("scalar_parameters", "vector_parameters"):
        params = mpc.get_editor_property(prop)
        for param in params:
            name = str(param.get_editor_property("parameter_name"))
            if name in values:
                value = values.pop(name)
                if isinstance(value, (tuple, list)):
                    value = unreal.LinearColor(*value)
                param.set_editor_property("default_value", value)
        mpc.set_editor_property(prop, params)
    if values:
        raise RuntimeError(f"{toon_spec.WORLD_MPC} has no parameter(s) {sorted(values)}; rerun create_toon_assets.py")
    if not EAL.save_asset(toon_spec.WORLD_MPC, only_if_is_dirty=False):
        raise RuntimeError(f"could not save {toon_spec.WORLD_MPC}")


def main():
    outline, mpc = load(toon_spec.OUTLINE_MATERIAL), load(toon_spec.WORLD_MPC)
    presets = toon_spec.load_world_presets()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for map_path, preset in toon_spec.LOOK_MAPS.items():
        if not level.load_level(map_path):
            raise RuntimeError(f"could not load {map_path}")
        place_volume(actors, outline, preset)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        if not level.save_current_level():
            raise RuntimeError(f"could not save {map_path}")
        unreal.log(f"{map_path} ({world.get_name()}): look volume {preset} placed")
    write_mpc_defaults(mpc, presets, DEFAULT_PRESET)
    unreal.log(f"{toon_spec.WORLD_MPC}: {DEFAULT_PRESET} defaults saved")


main()
