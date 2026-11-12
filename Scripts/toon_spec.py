"""Engine-free data and helpers for the toon look (M_TTA_Toon, MPC_TTA_World).

The editor scripts (create_toon_assets.py, swap_to_toon.py) import this; it is unit-tested by `make test`.
World presets live in Config/DefaultGame.ini so the C++ runtime (UTTAWorldLookSettings) and these
scripts read the same values.
"""
import re
import struct
import zlib
from pathlib import Path

TOON_DIR = "/Game/Art/Toon"
TOON_MATERIAL = f"{TOON_DIR}/M_TTA_Toon"
OUTLINE_MATERIAL = f"{TOON_DIR}/M_TTA_PP_Outline"
WORLD_MPC = f"{TOON_DIR}/MPC_TTA_World"
PALETTE_TEXTURE = f"{TOON_DIR}/T_TTA_Palette"
INSTANCE_DIR = f"{TOON_DIR}/Instances"
ART_TEST_MAP = "/Game/Maps/TTA_ArtTest"

# Shared 12-colour palette. Index i sits in texel column i of the 12x1 palette texture.
PALETTE = (
    ("Ink", "1E1B2E"), ("Bone", "F4F1E8"), ("Red", "E5484D"), ("Orange", "F2913D"),
    ("Yellow", "F5D547"), ("Grass", "6CC24A"), ("Teal", "2BB3A3"), ("Sky", "5BA8F5"),
    ("Blue", "3F5BD9"), ("Purple", "8E5CD9"), ("Pink", "F27BB0"), ("Wood", "9A6A43"),
)
DEFAULT_INDEX = 1  # Bone: neutral when an imported material has no readable colour

# MPC_TTA_World scalar parameters, in the order the material graph uses them.
WORLD_PARAMS = ("Saturation", "Contrast", "ShadowFloor", "Bands", "RimStrength", "Warmth")
_INI_SECTION = "[/Script/TakeTwoAfterlife.TTAWorldLookSettings]"
DEFAULT_GAME_INI = Path(__file__).resolve().parents[1] / "Config" / "DefaultGame.ini"


def hex_to_rgb(code):
    if not re.fullmatch(r"[0-9A-Fa-f]{6}", code):
        raise ValueError(f"not a 6-digit hex colour: {code!r}")
    return tuple(int(code[i:i + 2], 16) for i in (0, 2, 4))


def palette_rgb():
    return [hex_to_rgb(code) for _, code in PALETTE]


def palette_u(index):
    """Texture U at the centre of palette texel `index` (what M_TTA_Toon samples)."""
    if not 0 <= index < len(PALETTE):
        raise ValueError(f"palette index {index} outside 0..{len(PALETTE) - 1}")
    return (index + 0.5) / len(PALETTE)


def palette_png():
    """The palette as a 12x1 RGB PNG (import with nearest filtering, no mips)."""
    raw = b"\x00" + bytes(c for rgb in palette_rgb() for c in rgb)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    header = struct.pack(">IIBBBBB", len(PALETTE), 1, 8, 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw))
            + chunk(b"IEND", b""))


def nearest_palette_index(rgb):
    """Closest palette entry to an sRGB 0-255 colour (squared distance)."""
    if len(rgb) != 3:
        raise ValueError(f"need an RGB triple, got {rgb!r}")
    colours = palette_rgb()
    return min(range(len(colours)), key=lambda i: sum((a - b) ** 2 for a, b in zip(colours[i], rgb)))


def instance_name(source_material_name):
    """MI name for an imported material swapped to M_TTA_Toon."""
    stem = re.sub(r"^(M|MI|Mat)_", "", source_material_name)
    stem = re.sub(r"[^A-Za-z0-9_]", "_", stem)
    if not stem:
        raise ValueError(f"no usable name in {source_material_name!r}")
    return f"MI_TTA_Toon_{stem}"


def load_world_presets(ini_path=DEFAULT_GAME_INI):
    """{'Heaven': {...}, 'Course': {...}} from the TTAWorldLookSettings section."""
    text = Path(ini_path).read_text(encoding="utf-8")
    if _INI_SECTION not in text:
        raise ValueError(f"{ini_path} has no {_INI_SECTION} section")
    body = text.split(_INI_SECTION, 1)[1].split("\n[", 1)[0]
    presets = {}
    for world, fields in re.findall(r"^(\w+)=\((.*)\)\s*$", body, re.M):
        values = dict(pair.split("=") for pair in fields.split(","))
        missing = set(WORLD_PARAMS) - set(values)
        if missing:
            raise ValueError(f"{world} preset is missing {sorted(missing)}")
        presets[world] = {name: float(values[name]) for name in WORLD_PARAMS}
    if set(presets) != {"Heaven", "Course"}:
        raise ValueError(f"need Heaven and Course presets, got {sorted(presets)}")
    return presets
