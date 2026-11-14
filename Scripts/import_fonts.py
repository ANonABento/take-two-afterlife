# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_fonts.py -unattended -nosplash
# Imports Lilita One and Nunito (OFL) as font assets the menu loads:
#   /Game/UI/Fonts/LilitaOne  and  /Game/UI/Fonts/Nunito
import unreal
from pathlib import Path

EAL = unreal.EditorAssetLibrary
FONT_DIR = "/Game/UI/Fonts"
DOWNLOADS = Path(__file__).resolve().parents[1] / "Downloads" / "Fonts"
SOURCES = (
    ("LilitaOne", DOWNLOADS / "LilitaOne-Regular.ttf"),
    ("Nunito", DOWNLOADS / "Nunito-Regular.ttf" if (DOWNLOADS / "Nunito-Regular.ttf").is_file()
     else DOWNLOADS / "Nunito-Variable.ttf"),
)


def import_face(name, src):
    if not src.is_file():
        raise RuntimeError(f"missing font file {src}")
    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = FONT_DIR
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    path = f"{FONT_DIR}/{name}"
    if EAL.does_asset_exist(path):
        return path
    # The factory sometimes keeps the file stem. Rename that asset onto the menu path.
    stem = src.stem.replace("-", "_").replace("[", "_").replace("]", "_")
    for asset_path in EAL.list_assets(FONT_DIR, recursive=False, include_folder=False):
        if name.lower() in asset_path.lower() or stem.lower() in asset_path.lower():
            if not EAL.rename_asset(asset_path, path):
                raise RuntimeError(f"could not rename {asset_path} to {path}")
            return path
    raise RuntimeError(f"font import did not create {path}")


def main():
    made = []
    for name, src in SOURCES:
        path = import_face(name, src)
        asset = EAL.load_asset(path)
        if asset is None:
            raise RuntimeError(f"{path} did not load")
        kind = asset.get_class().get_name()
        if kind not in ("FontFace", "Font"):
            raise RuntimeError(f"{path} is a {kind}, not a font")
        EAL.save_loaded_asset(asset)
        made.append(f"{path} ({kind}, from {src.name})")
    EAL.save_directory(FONT_DIR, only_if_is_dirty=False, recursive=True)
    unreal.log("imported fonts: " + "; ".join(made))
    # Font import needs Slate, so this runs in the full editor. Close it when the assets are saved.
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")


main()
