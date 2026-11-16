"""Rows for /Game/Data/DT_Cosmetics (row struct FTTACosmeticRow), engine-free.

Hat and face names come from Core/include/tta/MenuFlow.h kHats/kFaces so the table
row index matches the replicated PlayerState Hat/Face index. Row i holds hat i and
face i; the runner reads its hat fields from row Hat and its face texture from row Face.
"""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
MENU_FLOW = REPO / "Core" / "include" / "tta" / "MenuFlow.h"
MODELS = REPO / "models"
HAT_DIR = "/Game/Characters/Hats"
FACE_DIR = "/Game/Characters/Faces"
TABLE_DIR = "/Game/Data"
TABLE_NAME = "DT_Cosmetics"
ROW_STRUCT = "/Script/TakeTwoAfterlife.TTACosmeticRow"
# Default fit on SK_Angel's head socket; edit per row in the editor afterwards.
OFFSET = (0.0, 0.0, 12.0)
ROTATION = (0.0, 0.0, 0.0)
SCALE = (1.0, 1.0, 1.0)
# Hat tint, as indices into toon_spec.PALETTE / kPaletteHex.
HAT_PALETTE = {
    "BunnyEars": 10, "Crown": 4, "DevilHorns": 2, "Halo": 4,
    "PartyCone": 3, "PropellerBeanie": 7, "TrafficCone": 3, "VikingHelmet": 11,
}
FIELDS = ("Name", "HatMesh", "SocketOffset", "Rotation", "Scale", "FaceTexture", "PaletteIndex")


def core_names(array, text=None):
    text = MENU_FLOW.read_text(encoding="utf-8") if text is None else text
    m = re.search(r"\b%s\s*=\s*\{([^}]*)\}" % array, text)
    if not m:
        raise ValueError(f"{array} not found in {MENU_FLOW}")
    return re.findall(r'"([^"]+)"', m.group(1))


def asset_path(folder, name):
    return f"{folder}/{name}.{name}"


def build_rows(hats=None, faces=None):
    hats = core_names("kHats") if hats is None else hats
    faces = core_names("kFaces") if faces is None else faces
    if not hats or not faces:
        raise ValueError(f"{len(hats)} hats vs {len(faces)} faces")
    missing = [h for h in hats if h not in HAT_PALETTE]
    if missing:
        raise ValueError(f"no palette index for hats {missing}")
    # One row per index. Hat i and face i are read independently, so the table is as long as the longer list.
    count = max(len(hats), len(faces))
    rows = []
    for i in range(count):
        hat = hats[i] if i < len(hats) else hats[0]
        face = faces[i] if i < len(faces) else faces[0]
        rows.append({
            "Name": f"Look{i}",
            "HatMesh": asset_path(HAT_DIR, f"SM_Hat_{hat}"),
            "SocketOffset": OFFSET,
            "Rotation": ROTATION,
            "Scale": SCALE,
            "FaceTexture": asset_path(FACE_DIR, f"T_Face_{face}"),
            "PaletteIndex": HAT_PALETTE[hat],
        })
    return rows


def source_files(rows):
    """Imported asset for each row: the Content uasset, which is what the game loads."""
    out = []
    for row in rows:
        for key, folder in (("HatMesh", "Content/Characters/Hats"), ("FaceTexture", "Content/Characters/Faces")):
            name = row[key].rsplit("/", 1)[1].split(".")[0]
            out.append((REPO / folder / f"{name}.uasset", "/" + folder.replace("Content", "Game")))
    return out


def _cell(value):
    if isinstance(value, tuple) and len(value) == 3:
        return '"(X=%g,Y=%g,Z=%g)"' % value
    return str(value)


def _rot(value):
    return '"(Pitch=%g,Yaw=%g,Roll=%g)"' % value


def to_csv(rows):
    """CSV that DataTableFunctionLibrary.fill_data_table_from_csv_string accepts."""
    lines = ["---," + ",".join(FIELDS[1:])]
    for r in rows:
        cells = [r["HatMesh"], _cell(r["SocketOffset"]), _rot(r["Rotation"]), _cell(r["Scale"]),
                 r["FaceTexture"], str(r["PaletteIndex"])]
        lines.append(r["Name"] + "," + ",".join(cells))
    return "\n".join(lines) + "\n"
