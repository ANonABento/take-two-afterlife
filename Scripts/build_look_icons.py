"""Draw the settings hat icons. Faces already live on T_Face_* textures.

Writes Art/UI/Looks/T_UI_Hat_<Name>.png, one per tta::kHats entry.
The menu loads these at runtime, so they do not need an editor import.
"""
import os

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Art", "UI", "Looks")
S = 256


def canvas():
    image = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    return image, ImageDraw.Draw(image)


def bunny():
    image, draw = canvas()
    draw.ellipse((78, 150, 178, 230), fill=(255, 182, 214, 255))
    draw.ellipse((48, 28, 108, 190), fill=(255, 154, 196, 255))
    draw.ellipse((148, 28, 208, 190), fill=(255, 154, 196, 255))
    draw.ellipse((64, 52, 96, 160), fill=(255, 228, 240, 255))
    draw.ellipse((160, 52, 192, 160), fill=(255, 228, 240, 255))
    return image


def crown():
    image, draw = canvas()
    draw.polygon(
        [(36, 188), (52, 72), (92, 130), (128, 48), (164, 130), (204, 72), (220, 188)],
        fill=(245, 196, 55, 255),
    )
    draw.rectangle((36, 176, 220, 214), fill=(232, 168, 32, 255))
    for center in ((92, 118), (128, 86), (164, 118)):
        draw.ellipse((center[0] - 12, center[1] - 12, center[0] + 12, center[1] + 12), fill=(229, 72, 77, 255))
    draw.ellipse((116, 150, 140, 174), fill=(63, 91, 217, 255))
    return image


def horns():
    image, draw = canvas()
    draw.polygon([(70, 210), (88, 120), (40, 36), (108, 100), (118, 210)], fill=(196, 40, 48, 255))
    draw.polygon([(186, 210), (168, 120), (216, 36), (148, 100), (138, 210)], fill=(196, 40, 48, 255))
    return image


def halo():
    image, draw = canvas()
    draw.ellipse((28, 68, 228, 188), outline=(245, 213, 71, 255), width=28)
    draw.ellipse((56, 92, 200, 164), fill=(0, 0, 0, 0))
    return image


def party():
    image, draw = canvas()
    draw.polygon([(128, 28), (58, 210), (198, 210)], fill=(242, 123, 176, 255))
    draw.polygon([(128, 70), (86, 210), (128, 210)], fill=(245, 213, 71, 255))
    draw.polygon([(128, 120), (168, 210), (198, 210)], fill=(91, 168, 245, 255))
    draw.ellipse((108, 12, 148, 52), fill=(245, 213, 71, 255))
    return image


def beanie():
    image, draw = canvas()
    draw.ellipse((58, 150, 198, 214), fill=(43, 179, 163, 255))
    draw.chord((58, 78, 198, 200), 180, 360, fill=(43, 179, 163, 255))
    draw.rectangle((108, 70, 148, 108), fill=(245, 213, 71, 255))
    draw.polygon([(78, 88), (128, 36), (128, 96)], fill=(229, 72, 77, 255))
    draw.polygon([(178, 88), (128, 36), (128, 96)], fill=(63, 91, 217, 255))
    draw.ellipse((112, 20, 144, 52), fill=(245, 213, 71, 255))
    return image


def cone():
    image, draw = canvas()
    draw.polygon([(128, 24), (72, 214), (184, 214)], fill=(242, 145, 61, 255))
    draw.polygon([(128, 110), (96, 168), (160, 168)], fill=(244, 241, 232, 255))
    draw.rectangle((86, 200, 170, 222), fill=(242, 145, 61, 255))
    return image


def viking():
    image, draw = canvas()
    draw.chord((68, 70, 188, 220), 200, 340, fill=(154, 164, 178, 255))
    draw.rectangle((68, 150, 188, 196), fill=(154, 164, 178, 255))
    draw.polygon([(78, 150), (28, 48), (70, 96), (96, 150)], fill=(232, 168, 32, 255))
    draw.polygon([(178, 150), (228, 48), (186, 96), (160, 150)], fill=(232, 168, 32, 255))
    draw.rectangle((78, 168, 178, 188), fill=(90, 98, 112, 255))
    return image


HATS = {
    "BunnyEars": bunny,
    "Crown": crown,
    "DevilHorns": horns,
    "Halo": halo,
    "PartyCone": party,
    "PropellerBeanie": beanie,
    "TrafficCone": cone,
    "VikingHelmet": viking,
}


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, draw in HATS.items():
        path = os.path.join(OUT, "T_UI_Hat_%s.png" % name)
        draw().save(path)
        print(path)


if __name__ == "__main__":
    main()
