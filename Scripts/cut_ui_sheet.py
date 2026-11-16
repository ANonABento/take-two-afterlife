"""Cut menu chrome and icons out of the contact sheet.

The sheet is Art/UI/ref/assets_contact.png (flat #282828 background). Each sprite is
keyed to alpha, trimmed, and written to Art/UI/T_UI_*.png. Sliced pieces get 9-slice
margins in Art/UI/T_UI_Slices.json from the alpha silhouette. Icons are then centred
on the 256 canvas by normalize_ui_icons. The title backdrop is not on this sheet at
a useful size, so T_UI_TitleBackdrop.png is left as it is.

Run: python Scripts/cut_ui_sheet.py
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Scripts"))
import normalize_ui_icons as icons  # noqa: E402

ART = ROOT / "Art" / "UI"
SHEET_PATH = ART / "ref" / "assets_contact.png"
SLICES_NAME = "T_UI_Slices.json"
BG = np.array([40, 40, 40], dtype=np.float32)
# Boxes on the sheet, exclusive. Mic head and stand are one sprite.
BOXES = {
    "T_UI_Button": (0, 0, 384, 114),
    "T_UI_ButtonHover": (387, 0, 773, 123),
    "T_UI_ButtonPressed": (777, 0, 1163, 111),
    "T_UI_ButtonPrimary": (1167, 0, 1553, 132),
    "T_UI_Panel": (0, 135, 325, 441),
    "T_UI_CodePill": (393, 134, 717, 262),
    "T_UI_Banner": (720, 134, 1107, 224),
    "T_UI_SliderBar": (1110, 135, 1497, 169),
    "T_UI_SliderKnob": (0, 444, 132, 581),
    "T_UI_TextField": (134, 444, 522, 534),
    "T_UI_IconMicOn": (526, 445, 709, 751),
    "T_UI_IconMicOff": (714, 446, 928, 750),
    "T_UI_IconCamFixed": (933, 445, 1194, 672),
    "T_UI_IconCamPan": (1198, 445, 1460, 722),
    "T_UI_IconCamFollow": (0, 755, 259, 915),
    "T_UI_IconCamTrap": (263, 756, 525, 1044),
    "T_UI_IconReady": (529, 755, 791, 1021),
}
SLICED = {
    "T_UI_Button", "T_UI_ButtonHover", "T_UI_ButtonPressed", "T_UI_ButtonPrimary",
    "T_UI_Panel", "T_UI_CodePill", "T_UI_Banner", "T_UI_SliderBar", "T_UI_TextField",
}
ICONS = {name for name in BOXES if "Icon" in name}


def key(rgb):
    """Drop the flat background and un-blend the grey fringe."""
    pix = rgb.astype(np.float32)
    diff = np.abs(pix - BG).max(axis=2)
    alpha = np.clip((diff - 8.0) / 14.0, 0.0, 1.0)
    cover = alpha[..., None]
    fg = np.where(cover > 0.02, BG + (pix - BG) / np.maximum(cover, 0.02), 0)
    out = np.dstack([np.clip(fg, 0, 255), alpha * 255]).astype(np.uint8)
    return Image.fromarray(out, "RGBA")


def trim(img):
    box = icons.content_box(img)
    if box is None:
        raise RuntimeError("sprite has no visible pixels")
    return img.crop(box)


def _inset_per_column(mask):
    """For each x, how far down the first opaque pixel sits (height if the column is empty)."""
    h, w = mask.shape
    starts = []
    for x in range(w):
        ys = np.flatnonzero(mask[:, x])
        starts.append(int(ys[0]) if len(ys) else h)
    return starts


def _inset_per_row(mask):
    """For each y, how far in the first opaque pixel sits (width if the row is empty)."""
    h, w = mask.shape
    starts = []
    for y in range(h):
        xs = np.flatnonzero(mask[y])
        starts.append(int(xs[0]) if len(xs) else w)
    return starts


def _margin_from_profile(starts):
    """Index where the edge inset settles, before a notch in the middle of the side."""
    n = len(starts)
    level = float(np.median(starts[n // 4: 3 * n // 4]))
    run = 0
    hold = max(4, n // 48)
    for i, value in enumerate(starts):
        if abs(value - level) <= 2:
            run += 1
            if run >= hold:
                # The end of the run, so the last pixels of the curve stay in the cap.
                return max(1, i + 1)
        else:
            run = 0
    raise RuntimeError("edge never straightens")


def slice_margins(img):
    """Pixels of corner (and ribbon tail) that must not stretch."""
    mask = np.asarray(img)[:, :, 3] > 20
    h, w = mask.shape
    left = _margin_from_profile(_inset_per_column(mask))
    right = _margin_from_profile(_inset_per_column(mask[:, ::-1]))
    top = _margin_from_profile(_inset_per_row(mask))
    bottom = _margin_from_profile(_inset_per_row(mask[::-1, :]))
    if left + right >= w or top + bottom >= h:
        raise RuntimeError(f"slice margins ({left},{top},{right},{bottom}) cover a {w}x{h} sprite")
    return {"left": int(left), "top": int(top), "right": int(right), "bottom": int(bottom)}


def cut_sprite(sheet, box):
    x0, y0, x1, y1 = box
    rgb = np.asarray(sheet.crop((x0, y0, x1, y1)).convert("RGB"))
    return trim(key(rgb))


def build(out_dir=ART, sheet_path=SHEET_PATH):
    sheet = Image.open(sheet_path).convert("RGB")
    out_dir.mkdir(parents=True, exist_ok=True)
    assets = {}
    for name, box in BOXES.items():
        img = cut_sprite(sheet, box)
        if name in ICONS:
            img = icons.normalize(img)
        img.save(out_dir / f"{name}.png")
        entry = {"width": img.width, "height": img.height, "radius": 0}
        if name in SLICED:
            entry.update(slice_margins(img))
            # The ribbon tails sit on the vertical middle, so the auto top margin
            # swallows the white centre. Keep the tails in the side caps and let
            # the white plaque stretch.
            if name == "T_UI_Banner":
                entry.update(left=70, top=18, right=70, bottom=24)
            entry["radius"] = min(entry["left"], entry["top"], entry["right"])
        else:
            entry.update(dict.fromkeys(("left", "top", "right", "bottom"), 0))
        assets[name] = entry
    doc = {"outline": 0, "shadow": 0, "assets": assets}
    (out_dir / SLICES_NAME).write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    return doc


if __name__ == "__main__":
    for asset, entry in build()["assets"].items():
        print(asset, entry)
