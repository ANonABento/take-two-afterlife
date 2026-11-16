"""Draws the UI chrome textures (buttons, panels, pills, fields, slider) into Art/UI/.

Engine-free (Pillow). Each texture is a rounded rectangle in a palette colour from
Docs/design/ui-style.md with a 6 px Ink outline and a 4 px Ink drop shadow below it.
Corners are measured back from the drawn pixels; the 9-slice margins (corner radius +
outline, plus the shadow on the bottom edge) go to Art/UI/T_UI_Slices.json, which
UTTAUIStyle::Brush reads.

Run: python Scripts/build_ui_chrome.py, then Scripts/import_ui_assets.py in the editor.
"""
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "Art" / "UI"
STYLE_DOC = ROOT / "Docs" / "design" / "ui-style.md"
SLICES_NAME = "T_UI_Slices.json"

OUTLINE = 6
SHADOW = 4
SHADOW_ALPHA = 110
BUTTON_SIZE = (512, 160)

# name: (canvas size, corner radius, palette token, sliced)
CHROME = {
    "T_UI_Button": (BUTTON_SIZE, 32, "Bone", True),
    "T_UI_ButtonHover": (BUTTON_SIZE, 32, "Sky", True),
    "T_UI_ButtonPressed": (BUTTON_SIZE, 32, "Blue", True),
    "T_UI_ButtonPrimary": (BUTTON_SIZE, 32, "Yellow", True),
    "T_UI_Panel": ((256, 256), 32, "Bone", True),
    "T_UI_CodePill": ((384, 128), 56, "Yellow", True),
    "T_UI_Banner": ((512, 128), 24, "Bone", True),
    "T_UI_TextField": ((512, 128), 20, "Bone", True),
    "T_UI_SliderBar": ((512, 48), 16, "Sky", True),
    "T_UI_SliderKnob": ((96, 100), 48, "Yellow", False),
}


def palette():
    text = STYLE_DOC.read_text(encoding="utf-8")
    found = dict(re.findall(r"^\| `(\w+)` \| #([0-9A-Fa-f]{6}) \|", text, re.M))
    if "Ink" not in found:
        raise RuntimeError(f"no palette table in {STYLE_DOC}")
    return {k: tuple(int(v[i:i + 2], 16) for i in (0, 2, 4)) + (255,) for k, v in found.items()}


def draw(size, radius, fill, ink):
    """The face fills the canvas minus SHADOW px at the bottom; the shadow is the face moved down."""
    w, h = size
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    shadow = Image.new("RGBA", size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle((0, SHADOW, w - 1, h - 1), radius, fill=ink[:3] + (SHADOW_ALPHA,))
    img.alpha_composite(shadow)
    ImageDraw.Draw(img).rounded_rectangle((0, 0, w - 1, h - 1 - SHADOW), radius, fill=fill, outline=ink, width=OUTLINE)
    return img


def corner_extent(img):
    """Rows from the top until the left half stops curving: every later row matches the middle row."""
    w, h = img.size
    px = img.load()

    def row(y):
        return [px[x, y] for x in range(w // 2)]

    straight = row(h // 2)
    y = h // 2
    while y > 0 and row(y - 1) == straight:
        y -= 1
    return y


def margins(name, img, radius):
    """Slice margins in px. Fails if the drawn corner reaches past radius (slicing would cut it)."""
    measured = corner_extent(img)
    if measured > radius:
        raise RuntimeError(f"{name}: corner measures {measured} px, more than radius {radius}")
    edge = radius + OUTLINE
    return {"left": edge, "top": edge, "right": edge, "bottom": edge + SHADOW}


def build(out_dir=ART):
    colours = palette()
    out_dir.mkdir(parents=True, exist_ok=True)
    assets = {}
    for name, (size, radius, token, sliced) in CHROME.items():
        img = draw(size, radius, colours[token], colours["Ink"])
        img.save(out_dir / f"{name}.png")
        entry = {"width": size[0], "height": size[1], "radius": radius}
        entry.update(margins(name, img, radius) if sliced else dict.fromkeys(("left", "top", "right", "bottom"), 0))
        assets[name] = entry
    doc = {"outline": OUTLINE, "shadow": SHADOW, "assets": assets}
    (out_dir / SLICES_NAME).write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    return doc


if __name__ == "__main__":
    for asset, entry in build()["assets"].items():
        print(asset, entry)
