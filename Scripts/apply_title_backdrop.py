# Puts the menu painting on the Title map's Backdrop plane, stands the plane up facing the title
# camera, removes the ground plane that hides the painted meadow, and saves the level.
# Works on a Title map from any earlier create_title_map.py; opens Title itself.
# It reuses M_Hat_Textured (already in the project) instead of building a new material.
#   py C:/Users/kevbe/take-two-afterlife/Scripts/apply_title_backdrop.py
import os
import unreal

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PNG = os.path.join(REPO, "Art", "UI", "T_UI_TitleBackdrop.png")
TEX = "/Game/UI/Generated/T_UI_TitleBackdrop"
PARENT = "/Game/Characters/Hats/M_Hat_Textured"
MI = "/Game/UI/Generated/MI_TitleBackdrop"

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()

if not EAL.does_asset_exist(TEX):
    task = unreal.AssetImportTask()
    task.filename = PNG
    task.destination_path = "/Game/UI/Generated"
    task.destination_name = "T_UI_TitleBackdrop"
    task.automated = True
    task.replace_existing = True
    task.save = True
    AT.import_asset_tasks([task])

texture = EAL.load_asset(TEX)
parent = EAL.load_asset(PARENT)
if texture is None or parent is None:
    raise RuntimeError(f"need {TEX} and {PARENT}")

mi = EAL.load_asset(MI) if EAL.does_asset_exist(MI) else AT.create_asset(
    "MI_TitleBackdrop", "/Game/UI/Generated", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(mi, parent)
MEL.set_material_instance_texture_parameter_value(mi, "Tex", texture)
EAL.save_asset(MI)

TITLE_MAP = "/Game/Maps/Title"
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level.load_level(TITLE_MAP):
    raise RuntimeError(f"could not load {TITLE_MAP}; run create_title_map.py first")

actor_system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
found = False
for actor in actor_system.get_all_level_actors():
    label = actor.get_actor_label()
    if label == "Backdrop":
        # Rotator takes roll, pitch, yaw positionally, so name them. Yaw 90 + roll 90: U across the
        # screen, V down it, face toward the camera at -X.
        actor.set_actor_location_and_rotation(
            unreal.Vector(2600, 0, 400), unreal.Rotator(roll=90.0, pitch=0.0, yaw=90.0), False, True)
        actor.set_actor_scale3d(unreal.Vector(72, 48, 1))
        actor.static_mesh_component.set_material(0, mi)
        found = True
    elif label == "Ground":
        actor_system.destroy_actor(actor)
    elif label == "TitleCamera":
        actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-5.0, yaw=0.0), True)
if not found:
    raise RuntimeError("no actor labeled Backdrop in the Title level; run create_title_map.py first")
# Without the save the painting only lives in the open editor level and the game shows the old plane.
if not level.save_current_level():
    raise RuntimeError(f"could not save {TITLE_MAP}")
unreal.log("title backdrop painted and saved")
