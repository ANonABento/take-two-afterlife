# DT_Cosmetics builder: the ONLY script that writes /Game/Data/DT_Cosmetics.
# Run alone:  py "C:/Users/<you>/take-two-afterlife/Scripts/build_cosmetics_table.py"
# import_gen3_assets.py calls run() after importing the hats. Re-running overwrites row edits.
import os
import re
import sys
from pathlib import Path

import unreal

HERE = Path(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, str(HERE))
import cosmetics_rows as C  # noqa: E402

REPO = HERE.parent
EAL = unreal.EditorAssetLibrary
HATS = "/Game/Characters/Hats"
# Per-hat fit on the head socket: offset (cm), rotation (Pitch, Yaw, Roll), scale.
# Scale is one number or (X, Y, Z). Copied from the live editor adjustments.
DEFAULT_FIT = ((0.0, 0.0, 0.0), (180.0, 0.0, 270.0), 1.0)
HAT_FIT = {
    "Crown": ((0.0, 0.1, 0.0), (180.0, 0.0, 270.0), 1.33),
    "DevilHorns": ((0.0, 0.43, 0.0), (90.0, -180.0, 90.0), (2.1325, 2.0, 1.85)),
    "PartyCone": ((0.0, 0.06, -0.05), (0.0, -180.0, 106.0), 1.33),
    "TrafficCone": ((0.0, 0.06, 0.05), (0.0, -180.0, 76.0), 1.5275),
    "VikingHelmet": ((0.0, 0.4, 0.0), (0.0, -180.0, 90.0), (1.76, 1.9975, 1.5775)),
    "BunnyEars": ((0.0, 0.32, -0.01), (-90.0, -180.0, 90.0), (3.1025, 2.5525, 2.3325)),
}


def game_slots():
    text = (REPO / "Core" / "include" / "tta" / "MenuFlow.h").read_text(encoding="utf-8")
    hats = re.findall(r'"([^"]+)"', re.search(r"kHats\s*=\s*\{([^}]*)\}", text).group(1))
    faces = re.findall(r'"([^"]+)"', re.search(r"kFaces\s*=\s*\{([^}]*)\}", text).group(1))
    return hats, faces


def import_missing_faces(rows):
    tasks = []
    for src, folder in C.source_files(rows):
        if "SM_Hat_" in src.stem or EAL.does_asset_exist(f"{folder}/{src.stem}") or not src.is_file():
            continue
        t = unreal.AssetImportTask()
        t.filename, t.destination_path = str(src), folder
        t.automated, t.replace_existing, t.save = True, True, True
        tasks.append(t)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def run():
    hats, faces = game_slots()
    try:
        rows = C.build_rows(hats, faces)
    except TypeError:
        rows = C.build_rows()
    import_missing_faces(rows)
    report = []
    for r in rows:
        name = r["HatMesh"].rsplit("/", 1)[1].split(".")[0]
        hat = name.removeprefix("SM_Hat_")
        if not EAL.does_asset_exist(f"{HATS}/{name}"):
            unreal.log_warning(f"{name} not imported yet: run import_gen3_assets.py")
        off, rot, sc = HAT_FIT.get(hat, DEFAULT_FIT)
        scale = tuple(sc) if isinstance(sc, (tuple, list)) else (sc, sc, sc)
        r["SocketOffset"], r["Rotation"], r["Scale"] = off, rot, scale
        report.append(f"{r['Name']}: {hat} -> {name}")
    struct = unreal.load_object(None, C.ROW_STRUCT)
    if struct is None:
        raise RuntimeError(f"{C.ROW_STRUCT} not found: compile the game module first")
    path = f"{C.TABLE_DIR}/{C.TABLE_NAME}"
    table = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
    if table is None:
        f = unreal.DataTableFactory(); f.set_editor_property("struct", struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            C.TABLE_NAME, C.TABLE_DIR, unreal.DataTable, f)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, C.to_csv(rows)):
        raise RuntimeError(f"{path}: CSV fill failed")
    EAL.save_asset(path)
    missing = [r["FaceTexture"] for r in rows if not EAL.does_asset_exist(r["FaceTexture"].split(".")[0])]
    return report, missing


if __name__ == "__main__":
    rep, miss = run()
    unreal.log("\n".join(["=== DT_Cosmetics ==="] + rep + ([f"MISSING faces: {miss}"] if miss else [])))
