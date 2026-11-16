# Run inside the UE editor (PythonScriptPlugin): (re)builds the Title map on every run.
# Title uses ATTATitleGameMode (no pawn, no build HUD; ATTATitlePlayerController shows the menu), is lit
# (sun, sky light, atmosphere, fog), has a toon backdrop and a CameraActor tagged TitleCamera that the
# title controller views. Without the override Title fell back to GlobalDefaultGameMode
# (ATTAGameMode + TTABuildHUD) in an empty, unlit level.
import unreal

TITLE_MAP = "/Game/Maps/Title"
TITLE_MODE = "/Script/TakeTwoAfterlife.TTATitleGameMode"
CAMERA_TAG = "TitleCamera"
PLANE = "/Engine/BasicShapes/Plane"
BACKDROP_TEXTURE = "/Game/UI/Generated/T_UI_TitleBackdrop"
BACKDROP_MATERIAL = "/Game/UI/Generated/M_TitleBackdrop"

EAL = unreal.EditorAssetLibrary
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if EAL.does_asset_exist(TITLE_MAP) and not EAL.delete_asset(TITLE_MAP):
    raise RuntimeError(f"could not delete old {TITLE_MAP} (open in the editor?)")
if not level.new_level(TITLE_MAP):
    raise RuntimeError(f"could not create {TITLE_MAP}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = unreal.load_class(None, TITLE_MODE)
if not mode:
    raise RuntimeError(f"{TITLE_MODE} not found: build the C++ module first")
world.get_world_settings().set_editor_property("default_game_mode", mode)


def spawn(cls, label, loc=(0, 0, 0), rot=(0.0, 0.0, 0.0)):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    actor.set_actor_label(label)
    return actor


def backdrop_material():
    """Unlit, two-sided material of the menu painting. Skipped when the editor material API isn't there."""
    if not hasattr(unreal, "MaterialEditingLibrary"):
        return EAL.load_asset(BACKDROP_MATERIAL) if EAL.does_asset_exist(BACKDROP_MATERIAL) else None
    if not EAL.does_asset_exist(BACKDROP_TEXTURE):
        from pathlib import Path
        task = unreal.AssetImportTask()
        task.filename = str(Path(__file__).resolve().parents[1] / "Art" / "UI" / "T_UI_TitleBackdrop.png")
        task.destination_path = "/Game/UI/Generated"
        task.destination_name = "T_UI_TitleBackdrop"
        task.automated = True
        task.replace_existing = True
        task.save = True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = EAL.load_asset(BACKDROP_TEXTURE)
    if texture is None:
        unreal.log_warning(f"title backdrop texture missing at {BACKDROP_TEXTURE}")
        return None
    folder, name = BACKDROP_MATERIAL.rsplit("/", 1)
    material = EAL.load_asset(BACKDROP_MATERIAL) if EAL.does_asset_exist(BACKDROP_MATERIAL) else None
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    mel.delete_all_material_expressions(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    sample = mel.create_material_expression(material, unreal.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property("texture", texture)
    mel.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    EAL.save_loaded_asset(material)
    return material


def plane(label, loc, rot, scale, material):
    actor = spawn(unreal.StaticMeshActor, label, loc, rot)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    mesh = actor.static_mesh_component
    mesh.set_static_mesh(EAL.load_asset(PLANE))
    mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if material:
        mesh.set_material(0, material)
    return actor


spawn(unreal.DirectionalLight, "Sun", (0, 0, 1000), (0.0, -40.0, 30.0))
spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
spawn(unreal.ExponentialHeightFog, "HeightFog")
# Stood up facing the title camera. Far enough that the whole choir (out to x~1520) is in front of it.
# Scale covers a 90 degree view: 3:2 painting, wider than 16:9 so the sides aren't sky.
plane("Backdrop", (2600, 0, 400), (-90.0, 180.0, 0.0), (48, 72, 1), backdrop_material())

camera = spawn(unreal.CameraActor, "TitleCamera", (-200, 0, 250), (-5.0, 0.0, 0.0))
camera.set_editor_property("tags", [CAMERA_TAG])

if not level.save_current_level():
    raise RuntimeError(f"could not save {TITLE_MAP}")
unreal.log(f"created {TITLE_MAP}")
