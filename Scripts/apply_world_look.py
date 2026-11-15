# Run inside the UE editor (PythonScriptPlugin):
#   py Scripts/apply_world_look.py
# For each map in toon_spec.LOOK_MAPS: removes earlier look volumes (tag TTAWorldLook), places one unbound
# PostProcessVolume with M_TTA_PP_Outline as a blendable, tagged with its preset name, sets the preset's
# MPC_TTA_World values in the editor world (UTTAWorldLookLibrary::ApplyWorldLook sets them in game), and saves.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import toon_spec  # noqa: E402

EAL = unreal.EditorAssetLibrary


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


def main():
    outline, mpc = load(toon_spec.OUTLINE_MATERIAL), load(toon_spec.WORLD_MPC)
    presets = toon_spec.load_world_presets()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for map_path, preset in toon_spec.LOOK_MAPS.items():
        if not level.load_level(map_path):
            raise RuntimeError(f"could not load {map_path}")
        place_volume(actors, outline, preset)
        world = unreal.EditorLevelLibrary.get_editor_world()
        for name, value in presets[preset].items():
            unreal.KismetMaterialLibrary.set_scalar_parameter_value(world, mpc, name, value)
        if not level.save_current_level():
            raise RuntimeError(f"could not save {map_path}")
        unreal.log(f"{map_path}: look {preset} applied")


main()
