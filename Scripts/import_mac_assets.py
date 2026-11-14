# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_mac_assets.py -unattended -nosplash
# The plan branch has no Assets/ folder. Course, heaven dressing and backyard props live under models/;
# the synth pack lives under Audio/SFX/. Meshes import in centimetres (unit conversion, scale 1) with
# collision. Each wav replaces the silent SoundWave of the matching cue id.
import sys
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[1]
EAL = unreal.EditorAssetLibrary

# snake_case wav -> cue id the game loads (plan handoff + Core/src/AudioCues.cpp).
SFX_HOOK = {
    "trap_place": "SFX_TrapPlace",
    "countdown_beep": "SFX_CountdownBeep",
    "clock_tick": "SFX_ClockTick",
    "bonk": "SFX_ImpactBonk",
    "thunder": "SFX_Thunder",
    "gate_chime": "SFX_GateChime",
    "projector_start": "SFX_ProjectorStart",
    "rating_ding": "SFX_RatingDing",
    "applause": "SFX_Applause",
    "fanfare": "SFX_Fanfare",
}


def mesh_sources():
    """(fbx path, destination folder). FBX only, so the glb twin of the same mesh is skipped."""
    groups = (
        ("SM_Course_", "/Game/Art/Course"),
        ("SM_Heaven_", "/Game/Art/Heaven"),
        ("SM_Backyard_", "/Game/Art/Props"),
    )
    # Cloud, couch and screen are already in /Game/Art/Heaven at the cinema's scale.
    keep = {"SM_Heaven_Cloud", "SM_Heaven_Couch", "SM_Heaven_Screen"}
    found = []
    for prefix, dest in groups:
        for path in sorted((REPO / "models").glob(prefix + "*.fbx")):
            if path.stem in keep:
                continue
            found.append((path, dest))
    assets = REPO / "Assets"
    if assets.is_dir():
        for folder, dest in (("Course", "/Game/Art/Course"), ("Heaven", "/Game/Art/Heaven"),
                             ("Props", "/Game/Art/Props")):
            root = assets / folder
            if not root.is_dir():
                continue
            for path in sorted(root.rglob("*")):
                if path.suffix.lower() in {".fbx", ".glb", ".obj"}:
                    found.append((path, dest))
    return found


def fbx_options(scale):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.import_animations = False
    ui.import_materials = False
    ui.import_textures = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    data = ui.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = True
    data.generate_lightmap_u_vs = True
    data.convert_scene = True
    data.convert_scene_unit = scale == 1.0
    data.import_uniform_scale = scale
    return ui


def import_meshes(scale):
    tasks = []
    for src, dest in mesh_sources():
        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest
        task.destination_name = src.stem
        task.automated = True
        task.replace_existing = True
        task.save = True
        if src.suffix.lower() == ".fbx":
            task.options = fbx_options(scale)
        tasks.append(task)
    if not tasks:
        raise RuntimeError("no course/heaven/prop meshes to import")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    return len(tasks)


def mesh_size(path):
    mesh = EAL.load_asset(path)
    if mesh is None:
        raise RuntimeError(f"import failed for {path}")
    box = mesh.get_bounding_box()
    return box.max - box.min


def ensure_collision(path):
    mesh = EAL.load_asset(path)
    body = mesh.get_editor_property("body_setup")
    count = 0
    if body is not None:
        agg = body.get_editor_property("agg_geom")
        for prop in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems"):
            try:
                count += len(agg.get_editor_property(prop))
            except Exception:
                pass
    if count == 0:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
        EAL.save_loaded_asset(mesh)


def import_sfx():
    folder = REPO / "Audio" / "SFX"
    wavs = sorted(folder.glob("*.wav"))
    if not wavs:
        raise RuntimeError(f"no wavs in {folder}")
    tasks = []
    hooked = []
    for src in wavs:
        name = SFX_HOOK.get(src.stem)
        if name is None:
            unreal.log_warning(f"no cue for {src.name}; importing under its own name")
            name = src.stem
        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = "/Game/Audio/SFX"
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
        hooked.append((src.name, name))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for src_name, name in hooked:
        wave = EAL.load_asset(f"/Game/Audio/SFX/{name}")
        if wave is None:
            raise RuntimeError(f"sfx import failed for {name}")
        duration = float(wave.get_editor_property("duration"))
        unreal.log(f"hooked {src_name} -> {name} ({duration:.2f}s)")
        if duration < 0.05:
            raise RuntimeError(f"{name} is still silent ({duration}s)")
    return len(hooked)


def main():
    count = import_meshes(1.0)
    size = mesh_size("/Game/Art/Course/SM_Course_Straight")
    unreal.log(f"SM_Course_Straight size cm: {size.x:.1f} {size.y:.1f} {size.z:.1f}")
    # Blender authored the kit in metres. Unit conversion should land the 4 m plank at 400 cm.
    if size.x < 50.0:
        unreal.log_warning("course imported in metres; reimporting at centimetre scale")
        count = import_meshes(100.0)
        size = mesh_size("/Game/Art/Course/SM_Course_Straight")
        unreal.log(f"SM_Course_Straight size cm after rescale: {size.x:.1f} {size.y:.1f} {size.z:.1f}")
    if not (350.0 <= size.x <= 450.0 and 150.0 <= size.y <= 250.0):
        raise RuntimeError(f"course scale is wrong: {size.x:.1f} x {size.y:.1f} x {size.z:.1f} cm")
    for src, dest in mesh_sources():
        ensure_collision(f"{dest}/{src.stem}")
    sounds = import_sfx()
    for folder in ("/Game/Art/Course", "/Game/Art/Heaven", "/Game/Art/Props", "/Game/Audio/SFX"):
        EAL.save_directory(folder, only_if_is_dirty=False, recursive=True)
    unreal.log(f"imported {count} mac meshes and hooked {sounds} sfx")


main()
