# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd <uproject> -run=pythonscript -script=Scripts/import_audio.py
# Imports every WAV in Content/Audio/<Music|SFX|VO>/ (built by build_audio.py, VO from VO_wavs.zip)
# over the /Game/Audio asset of the same name, replacing the silent placeholders, and sets Looping
# on the MUS_ waves. Set TTA_AUDIO_SRC to read the WAVs from another folder with the same layout.
import os
import sys
import wave
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
import placeholder_audio as P  # noqa: E402

EAL = unreal.EditorAssetLibrary
SRC = Path(os.environ.get("TTA_AUDIO_SRC") or P.REPO / "Content" / "Audio")

found = [(cid, folder, SRC / folder / f"{cid}.wav") for cid, folder in P.cues()]
found = [c for c in found if c[2].exists()]
if not found:
    raise RuntimeError(f"import_audio: no cue WAVs under {SRC}")

tasks = []
for cid, folder, wav in found:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(wav))
    task.set_editor_property("destination_path", P.package_dir(folder))
    task.set_editor_property("destination_name", cid)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

bad = []
for cid, folder, wav in found:
    sound = EAL.load_asset(P.asset_path(cid, folder))
    with wave.open(str(wav)) as w:
        seconds = w.getnframes() / w.getframerate()
    # the asset must now have the WAV's length (placeholders are 0.25 s of silence)
    if not sound or abs(sound.get_editor_property("duration") - seconds) > 0.05:
        bad.append(cid)
        continue
    if folder == "Music":
        sound.set_editor_property("looping", True)
    EAL.save_loaded_asset(sound, only_if_is_dirty=False)
if bad:
    raise RuntimeError(f"import_audio: {len(bad)} sounds not replaced, first: {bad[0]}")
unreal.log(f"import_audio: {len(found)} sounds imported from {SRC}")
