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
TOON_MATERIAL = "/Game/Art/Toon/M_TTA_Toon"

EAL = unreal.EditorAssetLibrary
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Reruns load the existing map and empty it. Never delete_asset the map and new_level the same
# path: force-deleting a world (worst when it is the open level) leaves half-dead objects that
# the new /Game/Maps/Title collides with, and the editor dies (Class.cpp:2640 assert in
# DeleteAsset, or an access violation inside new_level).
if EAL.does_asset_exist(TITLE_MAP):
    if not level.load_level(TITLE_MAP):
        raise RuntimeError(f"could not load {TITLE_MAP}")
    for old in actors.get_all_level_actors():
        actors.destroy_actor(old)
elif not level.new_level(TITLE_MAP):
    raise RuntimeError(f"could not create {TITLE_MAP}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = unreal.load_class(None, TITLE_MODE)
if not mode:
    raise RuntimeError(f"{TITLE_MODE} not found: build the C++ module first")
world.get_world_settings().set_editor_property("default_game_mode", mode)


def spawn(cls, label, loc=(0, 0, 0), roll=0.0, pitch=0.0, yaw=0.0):
    # Rotator takes roll, pitch, yaw positionally, so always name them.
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    actor.set_actor_label(label)
    return actor


def plane(label, loc, scale, **rot):
    actor = spawn(unreal.StaticMeshActor, label, loc, **rot)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    mesh = actor.static_mesh_component
    mesh.set_static_mesh(EAL.load_asset(PLANE))
    if EAL.does_asset_exist(TOON_MATERIAL):
        mesh.set_material(0, EAL.load_asset(TOON_MATERIAL))
    return actor


spawn(unreal.DirectionalLight, "Sun", (0, 0, 1000), pitch=-40.0, yaw=30.0)
spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
spawn(unreal.SkyAtmosphere, "SkyAtmosphere")
spawn(unreal.ExponentialHeightFog, "HeightFog")
# Stood up facing the title camera, past the choir (angels spawn out to about x=1520): yaw 90 +
# roll 90 puts the plane's U across the screen, V down it and its face toward -X. 72 x 48 m is the
# painting's 3:2. No ground plane: it would cover the painted meadow.
plane("Backdrop", (2600, 0, 400), (72, 48, 1), roll=90.0, yaw=90.0)

camera = spawn(unreal.CameraActor, "TitleCamera", (-200, 0, 250), pitch=-5.0)
camera.set_editor_property("tags", [CAMERA_TAG])

if not level.save_current_level():
    raise RuntimeError(f"could not save {TITLE_MAP}")
unreal.log(f"created {TITLE_MAP}")
