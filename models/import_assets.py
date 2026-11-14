"""Task 8 (phase 2): import every third-party / scripted asset Take Two Afterlife references, then report missing refs.

Run inside the UE 5.5 editor (Python Editor Script Plugin on), BEFORE Scripts/build_backyard_map.py:
    py models/import_assets.py <downloads folder>
<downloads> layout (one folder per source, files named exactly as the game expects):
    KayKitPrototypeBits/ KenneyCitySuburban/ KenneyNature/   unzipped packs (build_backyard_map.py imports these)
    Fonts/Nunito-Regular.ttf  Fonts/Nunito-Bold.ttf          Google Fonts, OFL 1.1
    Audio/VO/VO_*.wav  Audio/Music/MUS_*.wav  Audio/SFX/SFX_*.wav   (ids = Core/src/AudioCues.cpp)
Angel FBX files come from models/ in this repo.
Exit: prints "MISSING n" and raises if n > 0, so a CI/commandlet run fails loudly.
"""
import re
import sys
from pathlib import Path

import unreal

REPO = Path(__file__).resolve().parents[1]
EAL = unreal.EditorAssetLibrary
ANGEL_DIR = "/Game/Characters/Angel"
FONT_DIR = "/Game/UI/Fonts"
AUDIO_ROOT = "/Game/Audio"
KIND_FOLDER = {"Narrator": "VO", "Music": "Music", "Sfx": "SFX"}  # mirrors tta::assetPath
# name, bone, offset cm (UE: X fwd, Y right, Z up). From models/build_angel.py SOCKET_* empties (m -> cm, relative to bone head).
SOCKETS = (("HatSocket", "head", (0, 0, 49)), ("HaloSocket", "head", (0, 0, 62)), ("WingsSocket", "spine", (-12, 0, 10)))


def audio_cues():
    text = (REPO / "Core" / "src" / "AudioCues.cpp").read_text(encoding="utf-8")
    return re.findall(r'\{"((?:VO|MUS|SFX)_\w+)", AudioKind::(\w+)', text)


def run_tasks(tasks):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def task(src, dest, options=None, name=None):
    t = unreal.AssetImportTask()
    t.filename, t.destination_path = str(src), dest
    t.automated, t.replace_existing, t.save = True, True, True
    if name:
        t.destination_name = name
    if options:
        t.options = options
    return t


def import_angel():
    ui = unreal.FbxImportUI()
    ui.import_mesh, ui.import_as_skeletal, ui.import_animations = True, True, False
    ui.import_materials, ui.import_textures = False, False
    ui.create_physics_asset = True            # SK_Angel_PhysicsAsset: auto capsules per bone, used by the ragdoll
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.skeletal_mesh_import_data.import_uniform_scale = 1.0
    tasks = [task(REPO / "models" / "SK_Angel.fbx", ANGEL_DIR, ui)]
    for part in ("SM_Angel_Halo", "SM_Angel_Wings", "SM_Hat_Chef", "SM_Hat_Cowboy", "SM_Hat_Firefighter",
                 "SM_Hat_HardHat", "SM_Hat_Pilot", "SM_Hat_Police"):
        s = unreal.FbxImportUI()
        s.import_mesh, s.import_as_skeletal, s.import_materials = True, False, False
        s.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        tasks.append(task(REPO / "models" / f"{part}.fbx", ANGEL_DIR if "Angel" in part else "/Game/Characters/Hats", s))
    run_tasks(tasks)

    sk = EAL.load_asset(f"{ANGEL_DIR}/SK_Angel")
    if sk is None:
        return
    if sk.get_editor_property("physics_asset") is None:
        pa = EAL.load_asset(f"{ANGEL_DIR}/SK_Angel_PhysicsAsset")
        if pa:
            sk.set_editor_property("physics_asset", pa)
    have = {s.get_editor_property("socket_name") for s in sk.get_editor_property("skeleton").get_editor_property("sockets")} \
        if hasattr(sk.get_editor_property("skeleton"), "sockets") else set()
    for name, bone, offset in SOCKETS:
        if name in have:
            continue
        sock = unreal.SkeletalMeshSocket(outer=sk)
        sock.set_editor_property("socket_name", name)
        sock.set_editor_property("bone_name", bone)
        sock.set_editor_property("relative_location", unreal.Vector(*offset))
        sk.add_socket(sock, True)             # bAddToSkeleton so hats/halo attach on any mesh sharing the skeleton
    EAL.save_loaded_asset(sk)


def import_fonts(downloads):
    faces = {}
    tasks = []
    for style in ("Regular", "Bold"):
        src = Path(downloads) / "Fonts" / f"Nunito-{style}.ttf"
        if src.exists():
            tasks.append(task(src, FONT_DIR, name=f"FF_Nunito_{style}"))
    run_tasks(tasks)
    for style in ("Regular", "Bold"):
        faces[style] = EAL.load_asset(f"{FONT_DIR}/FF_Nunito_{style}")
    if not all(faces.values()):
        return
    path = f"{FONT_DIR}/F_TTA"
    font = EAL.load_asset(path) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "F_TTA", FONT_DIR, unreal.Font, unreal.FontFactory())
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
    typefaces = []
    for style, face in faces.items():          # TTAMenuWidget picks TypefaceFontName "Regular" / "Bold"
        e = unreal.TypefaceEntry()
        e.set_editor_property("name", style)
        fd = unreal.FontData()
        fd.set_editor_property("font_face_asset", face)
        e.set_editor_property("font", fd)
        typefaces.append(e)
    tf = unreal.Typeface()
    tf.set_editor_property("fonts", typefaces)
    cf = unreal.CompositeFont()
    cf.set_editor_property("default_typeface", tf)
    font.set_editor_property("composite_font", cf)
    EAL.save_loaded_asset(font)


def import_audio(downloads):
    tasks = []
    for cue_id, kind in audio_cues():
        folder = KIND_FOLDER[kind]
        src = Path(downloads) / "Audio" / folder / f"{cue_id}.wav"
        if src.exists():
            tasks.append(task(src, f"{AUDIO_ROOT}/{folder}", name=cue_id))
    run_tasks(tasks)
    for cue_id, kind in audio_cues():
        if kind == "Music":
            wave = EAL.load_asset(f"{AUDIO_ROOT}/Music/{cue_id}")
            if wave:
                wave.set_editor_property("looping", True)
                EAL.save_loaded_asset(wave)


def expected_refs():
    """Every /Game asset the code, map script and this importer depend on."""
    sys.path.insert(0, str(REPO / "Scripts"))
    import backyard_layout as L
    import toon_spec
    refs = [f"{ANGEL_DIR}/SK_Angel", f"{ANGEL_DIR}/SK_Angel_PhysicsAsset", f"{ANGEL_DIR}/SM_Angel_Halo",
            f"{ANGEL_DIR}/SM_Angel_Wings", f"{FONT_DIR}/F_TTA", toon_spec.TOON_MATERIAL]
    refs += [f"{AUDIO_ROOT}/{KIND_FOLDER[k]}/{i}" for i, k in audio_cues()]
    refs += [f"{L.IMPORT_ROOT}/{kit}/{name}" for kit, names in L.meshes_by_kit().items() for name in names]
    return refs


def check():
    missing = [r for r in expected_refs() if not EAL.does_asset_exist(r)]
    sk = EAL.load_asset(f"{ANGEL_DIR}/SK_Angel")
    if sk:
        names = {str(sk.find_socket(n).get_editor_property("socket_name")) for n, _, _ in SOCKETS if sk.find_socket(n)}
        missing += [f"{ANGEL_DIR}/SK_Angel:{n}" for n, _, _ in SOCKETS if n not in names]
    for m in missing:
        unreal.log_error(f"missing {m}")
    print(f"MISSING {len(missing)}")
    return missing


def main(downloads, check_only=False):
    if not check_only:
        import_angel()
        import_fonts(downloads)
        import_audio(downloads)
    if check():
        raise RuntimeError("asset refs missing, see log")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("usage: import_assets.py <downloads folder> [--check]")
    main(sys.argv[1], "--check" in sys.argv)
