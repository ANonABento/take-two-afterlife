# Puts the menu painting on the Title map's Backdrop plane.
# Run this only after create_title_map.py has finished and the Title level is open.
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

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
found = False
for actor in actors:
    if actor.get_actor_label() == "Backdrop":
        actor.static_mesh_component.set_material(0, mi)
        found = True
if not found:
    raise RuntimeError("no actor labeled Backdrop in the open level; run create_title_map.py first")
unreal.log("title backdrop painted")
