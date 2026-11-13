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
BACKDROP_MATERIAL = "/Game/Art/Toon/M_TTA_Toon"

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


def plane(label, loc, rot, scale):
    actor = spawn(unreal.StaticMeshActor, label, loc, rot)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    mesh = actor.static_mesh_component
    mesh.set_static_mesh(EAL.load_asset(PLANE))
    if EAL.does_asset_exist(BACKDROP_MATERIAL):
        mesh.set_material(0, EAL.load_asset(BACKDROP_MATERIAL))
    return actor


spawn(unreal.DirectionalLight, "Sun", (0, 0, 1000), (0.0, -40.0, 30.0))
spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
spawn(unreal.ExponentialHeightFog, "HeightFog")
plane("Backdrop", (1200, 0, 600), (0.0, 0.0, 90.0), (40, 40, 1))  # plane stood up, facing the camera
plane("Ground", (600, 0, 0), (0.0, 0.0, 0.0), (40, 40, 1))

camera = spawn(unreal.CameraActor, "TitleCamera", (-200, 0, 250), (-5.0, 0.0, 0.0))
camera.set_editor_property("tags", [CAMERA_TAG])

if not level.save_current_level():
    raise RuntimeError(f"could not save {TITLE_MAP}")
unreal.log(f"created {TITLE_MAP}")
