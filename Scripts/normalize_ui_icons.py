"""Trim empty edges off every Art/UI/T_UI_Icon*.png and centre it on a square canvas.

Engine-free (Pillow). Usage: python Scripts/normalize_ui_icons.py [dir]
"""
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "Art" / "UI"
PATTERN = "T_UI_Icon*.png"
CANVAS = 256
PADDING = 24
# Image-gen output leaves faint alpha noise around the glyph; ignore it when trimming.
ALPHA_THRESHOLD = 16


def content_box(img):
    """Bounding box of pixels with alpha above ALPHA_THRESHOLD, or None."""
    alpha = img.convert("RGBA").getchannel("A")
    return alpha.point(lambda a: 255 if a > ALPHA_THRESHOLD else 0).getbbox()


def normalize(img):
    img = img.convert("RGBA")
    box = content_box(img)
    if box is None:
        raise ValueError("icon has no visible pixels")
    glyph = img.crop(box)
    inner = CANVAS - 2 * PADDING
    scale = inner / max(glyph.size)
    size = (max(1, round(glyph.width * scale)), max(1, round(glyph.height * scale)))
    glyph = glyph.resize(size, Image.LANCZOS)
    out = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    out.alpha_composite(glyph, ((CANVAS - size[0]) // 2, (CANVAS - size[1]) // 2))
    return out


def run(folder=ART):
    paths = sorted(Path(folder).glob(PATTERN))
    for path in paths:
        normalize(Image.open(path)).save(path)
    return paths


if __name__ == "__main__":
    for p in run(Path(sys.argv[1]) if len(sys.argv) > 1 else ART):
        print("normalized", p.name)
