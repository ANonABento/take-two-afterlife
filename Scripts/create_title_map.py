# Run inside the UE editor (PythonScriptPlugin): creates the empty Title map if it is missing.
import unreal

TITLE_MAP = "/Game/Maps/Title"

if unreal.EditorAssetLibrary.does_asset_exist(TITLE_MAP):
    unreal.log(f"{TITLE_MAP} already exists")
else:
    if not unreal.EditorLevelLibrary.new_level(TITLE_MAP):
        raise RuntimeError(f"could not create {TITLE_MAP}")
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"created {TITLE_MAP}")
