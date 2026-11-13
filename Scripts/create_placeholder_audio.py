# Run inside the UE editor (PythonScriptPlugin), via Scripts/setup_content.ps1.
# Imports a short silent SoundWave at every /Game/Audio path the game references (Core AudioCues
# table), so PIE and packaged builds load something until the real VO / music / SFX are imported.
# A real asset already in Content/Audio/<VO|Music|SFX>/ is never touched; re-runs only fill gaps.
import sys
import tempfile
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
import placeholder_audio as P  # noqa: E402

EAL = unreal.EditorAssetLibrary

all_cues = P.cues()
todo = [c for c in P.missing(all_cues) if not EAL.does_asset_exist(P.asset_path(*c))]
wav = P.write_silent_wav(Path(tempfile.mkdtemp()) / "tta_silence.wav")

tasks = []
for cid, folder in todo:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(wav))
    task.set_editor_property("destination_path", P.package_dir(folder))
    task.set_editor_property("destination_name", cid)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    tasks.append(task)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

failed = [P.asset_path(*c) for c in todo if not EAL.does_asset_exist(P.asset_path(*c))]
if failed:
    raise RuntimeError(f"placeholder import failed for {len(failed)} sounds, first: {failed[0]}")
for cid, folder in todo:
    if folder == "Music":
        sound = EAL.load_asset(P.asset_path(cid, folder))
        sound.set_editor_property("looping", True)
        EAL.save_loaded_asset(sound)
unreal.log(f"placeholder audio: {len(todo)} created, {len(all_cues) - len(todo)} kept")
