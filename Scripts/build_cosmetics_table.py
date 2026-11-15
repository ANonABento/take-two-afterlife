# Run inside the UE editor (PythonScriptPlugin), after compiling the game module:
#   UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/build_cosmetics_table.py -unattended -nosplash
# Imports models/SM_Hat_*.fbx and models/renders/T_Face_*.png when missing, then (re)builds
# /Game/Data/DT_Cosmetics (FTTACosmeticRow) from cosmetics_rows. Re-running overwrites row edits.
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cosmetics_rows as C  # noqa: E402

EAL = unreal.EditorAssetLibrary


def import_missing(rows):
    tasks = []
    for src, folder in C.source_files(rows):
        if EAL.does_asset_exist(f"{folder}/{src.stem}"):
            continue
        if not src.is_file():
            raise RuntimeError(f"missing source {src}")
        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = folder
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def build_table(rows):
    struct = unreal.load_object(None, C.ROW_STRUCT)
    if struct is None:
        raise RuntimeError(f"{C.ROW_STRUCT} not found: compile the game module first")
    path = f"{C.TABLE_DIR}/{C.TABLE_NAME}"
    table = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    if table is None:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            C.TABLE_NAME, C.TABLE_DIR, unreal.DataTable, factory)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, C.to_csv(rows)):
        raise RuntimeError(f"{path}: CSV fill failed")
    EAL.save_asset(path)
    unreal.log(f"{path}: {len(rows)} rows")


rows = C.build_rows()
import_missing(rows)
build_table(rows)
