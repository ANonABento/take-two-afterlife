# Run inside the UE editor (PythonScriptPlugin):
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_ui_assets.py -unattended -nosplash
# Imports the generated UI art (Art/UI/T_UI_*.png, alpha PNGs) as UI textures:
#   /Game/UI/Generated/T_UI_<Name>  (UserInterface2D, no mips, UI group, never streamed)
# UTTAUIStyle::Brush loads them by that name; Docs/design/ui-style.md lists every asset.
import unreal
from pathlib import Path

EAL = unreal.EditorAssetLibrary
DEST = "/Game/UI/Generated"
ART = Path(__file__).resolve().parents[1] / "Art" / "UI"


def import_png(src):
    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = DEST
    task.destination_name = src.stem
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    path = f"{DEST}/{src.stem}"
    texture = EAL.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"{src.name} did not import as a Texture2D at {path}")
    return path, texture


def make_ui_texture(texture):
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("srgb", True)


def main():
    sources = sorted(ART.glob("T_UI_*.png"))
    if not sources:
        raise RuntimeError(f"no T_UI_*.png in {ART}")
    for src in sources:
        path, texture = import_png(src)
        make_ui_texture(texture)
        if not EAL.save_loaded_asset(texture):
            raise RuntimeError(f"could not save {path}")
    unreal.log(f"imported {len(sources)} UI textures into {DEST}")


main()
