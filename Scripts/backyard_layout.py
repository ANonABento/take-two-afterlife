"""Engine-free layout of the Backyard map (task 17): corridor floor, set dressing, start, gate, kill plane.

Units are metres, +X runs from start to gate, +Z is up, the corridor floor top is at Z = 0.
Built on Scripts/maplib (grid, roles, kill plane, credits, validate); this file keeps only the Backyard itself.
Scripts/build_backyard_map.py places this in UE; `make test` checks it (bounds, clear route, credits).
"""
import difflib
from pathlib import Path

import maplib
from maplib import WORLD_KILL_Z, Box, Kit, Point, Prop, footprint, palette_index  # noqa: F401  (map API)

REPO = maplib.REPO
MAP_PATH = "/Game/Maps/Backyard"
IMPORT_ROOT = "/Game/Imported"
THIRDPARTY_ROOT = "/Game/ThirdParty"         # kits imported as uassets (Content/ThirdParty/<kit>)
MISS_REPORT = REPO / "Saved" / "backyard_miss_report.txt"

CORRIDOR_LENGTH, CORRIDOR_WIDTH = maplib.corridor()
HALF_WIDTH = CORRIDOR_WIDTH / 2

KITS = {
    "KayKitPrototypeBits": Kit("KayKit Prototype Bits 1.0", "Kay Lousberg",
                               "https://github.com/KayKit-Game-Assets/KayKit-Prototype-Bits-1.0", "CC0 1.0", True),
    "KenneyCitySuburban": Kit("City Kit (Suburban)", "Kenney", "https://kenney.nl/assets/city-kit-suburban",
                              "CC0 1.0", False),
    "KenneyNature": Kit("Nature Kit", "Kenney", "https://kenney.nl/assets/nature-kit", "CC0 1.0", False),
}

FLOOR_THICKNESS = 0.2
DITCH = 2.0                                  # open gap each side of the corridor, down to the kill plane
LAWN_WIDTH = 8.0
START = Point(1.0, 0.0, 0.1)                 # PlayerStart, on the patio deck
GATE = Point(CORRIDOR_LENGTH - 1.0, 0.0, 0.0)
GATE_TRIGGER_HALF = (0.5, HALF_WIDTH, 2.0)   # half extents of the finish trigger
KILL_PLANE = maplib.kill_plane(CORRIDOR_LENGTH)

# Corridor zones, start to gate: (from x, to x, palette name, reading of the route).
ZONES = (
    (0.0, 6.0, "Wood", "patio deck start"),
    (6.0, 24.0, "Grass", "front lawn"),
    (24.0, 36.0, "Yellow", "sandpit"),
    (36.0, 48.0, "Grass", "back lawn"),
    (48.0, CORRIDOR_LENGTH, "Bone", "stone patio to the gate"),
)


def _floor():
    props = []
    for x0, x1, colour, _ in ZONES:
        props.append(Prop("KayKitPrototypeBits", "Primitive_Floor", "floor", (x0 + x1) / 2, 0.0, -FLOOR_THICKNESS, 0,
                          colour, ("box", (x1 - x0, CORRIDOR_WIDTH, FLOOR_THICKNESS))))
    lawn_y = HALF_WIDTH + DITCH + LAWN_WIDTH / 2
    for side in (-1, 1):
        props.append(Prop("KayKitPrototypeBits", "Floor_Dirt", "floor", CORRIDOR_LENGTH / 2, side * lawn_y,
                          -FLOOR_THICKNESS, 0, "Grass", ("box", (CORRIDOR_LENGTH + 16.0, LAWN_WIDTH, FLOOR_THICKNESS))))
    return props


ROUTE_TAG = "TTARoute"  # ATTABotController::RouteTag: bots walk these markers start to gate


def _markers():
    """Bone stepping stones down the centre line every 4 m: the route reads at a glance."""
    return [Prop("KayKitPrototypeBits", "Coin_A", "marker", x, 0.0, 0.0, 0, "Bone", ("box", (0.6, 0.6, 0.03)))
            for x in range(4, int(CORRIDOR_LENGTH) - 3, 4)]


# Alternate branches: (leave marker x, rejoin marker x, side). Each bows out to |y| = 2.5 and back.
SHORTCUTS = ((16, 24, 1), (40, 48, -1))


def _shortcuts():
    """Flat stones (no TTARoute tag) for two optional side branches between route markers."""
    props = []
    for x0, x1, side in SHORTCUTS:
        for dx, y in ((1.5, 1.5), (3.0, 2.5), (5.0, 2.5), (6.5, 1.5)):
            props.append(Prop("KayKitPrototypeBits", "Coin_A", "shortcut", x0 + dx, side * y, 0.0, 0, "Sky",
                              ("box", (0.6, 0.6, 0.03))))
        assert x0 + 6.5 < x1
    return props


def _route_dressing(edge):
    """Garden clutter lining the route on the inner lawn strip, using meshes the layout already resolves."""
    pieces = ((9, 1, "KenneyNature", "rock_smallA", "Ink", 0.4), (13, -1, "KayKitPrototypeBits", "Barrel_A", "Blue", 0.8),
              (20, 1, "KayKitPrototypeBits", "Box_A", "Red", 0.8), (25, -1, "KenneyNature", "flower_redA", "Pink", 0.5),
              (31, -1, "KenneyNature", "plant_bushLarge", "Teal", 1.0), (37, 1, "KayKitPrototypeBits", "Box_B", "Purple", 0.8),
              (42, 1, "KenneyNature", "flower_yellowA", "Yellow", 0.5), (46, -1, "KenneyNature", "rock_smallA", "Ink", 0.4),
              (54, 1, "KayKitPrototypeBits", "Pallet_Small", "Wood", 0.8),
              (57, -1, "KayKitPrototypeBits", "Barrel_A", "Orange", 0.8))
    return [Prop(kit, mesh, "dressing", x, side * (edge + 2.5), 0.0, 0, colour, ("height", h))
            for x, side, kit, mesh, colour, h in pieces]


def _dressing():
    edge = HALF_WIDTH + DITCH                     # inner edge of each side lawn
    far = edge + LAWN_WIDTH
    props = []
    for i, x in enumerate(range(0, int(CORRIDOR_LENGTH) + 1, 4)):   # garden fence on both outer edges
        for side in (-1, 1):
            props.append(Prop("KenneyCitySuburban", "fence", "dressing", x, side * (far - 0.3), 0.0, 0,
                              "Bone", ("height", 1.2)))
    trees = ((10, -1, "tree-large", 6.0), (30, 1, "tree-large", 6.5), (44, -1, "tree-small", 4.0),
             (18, 1, "tree-small", 3.5), (52, 1, "tree-large", 6.0))
    for x, side, mesh, h in trees:
        props.append(Prop("KenneyCitySuburban", mesh, "dressing", x, side * (edge + 5.0), 0.0, 0, "Grass",
                          ("height", h)))
    for x, side in ((7, 1), (15, -1), (26, -1), (38, 1), (47, 1), (56, -1)):
        props.append(Prop("KenneyNature", "plant_bushLarge", "dressing", x, side * (edge + 1.0), 0.0, 0, "Teal",
                          ("height", 1.0)))
    for x, side, mesh, colour in ((12, 1, "flower_redA", "Red"), (21, -1, "flower_yellowA", "Yellow"),
                                  (33, 1, "flower_redA", "Pink"), (41, -1, "flower_yellowA", "Orange")):
        props.append(Prop("KenneyNature", mesh, "dressing", x, side * (edge + 0.8), 0.0, 0, colour, ("height", 0.5)))
    for x, side in ((23, 1), (36, -1)):
        props.append(Prop("KenneyNature", "rock_smallA", "dressing", x, side * (edge + 1.5), 0.0, 0, "Ink",
                          ("height", 0.4)))
    for x, side in ((5, -1), (50, -1)):
        props.append(Prop("KenneyCitySuburban", "planter", "dressing", x, side * (edge + 2.0), 0.0, 45, "Wood",
                          ("height", 0.8)))
    # Sandpit toys and garden clutter on the lawns beside the sandpit zone.
    for x, side, mesh, colour in ((27, 1, "Barrel_A", "Blue"), (29, -1, "Box_A", "Red"), (34, 1, "Box_B", "Purple"),
                                  (4, 1, "table_medium", "Wood"), (2, -1, "Pallet_Small", "Wood")):
        props.append(Prop("KayKitPrototypeBits", mesh, "dressing", x, side * (edge + 3.0), 0.0, 0, colour,
                          ("height", 0.8 if mesh != "table_medium" else 0.75)))
    # The house the gate leads into, behind the finish, and a neighbour's house behind the start.
    props.append(Prop("KenneyCitySuburban", "building-type-a", "dressing", CORRIDOR_LENGTH + 6.0, 0.0, 0.0, 180,
                      "Sky", ("height", 7.0)))
    props.append(Prop("KenneyCitySuburban", "building-type-c", "dressing", -7.0, 0.0, 0.0, 0, "Orange",
                      ("height", 7.0)))
    return props + _route_dressing(edge)


def _gate():
    return [Prop("KayKitPrototypeBits", "Primitive_Doorway", "gate", GATE.x, GATE.y, GATE.z, 0, "Red",
                 ("box", (1.0, CORRIDOR_WIDTH, 4.0)))]


def props():
    return _floor() + _markers() + _shortcuts() + _dressing() + _gate()


def downloads_dir(args):
    """Packs folder: the first script arg, else <repo>/Downloads (filled by Scripts/fetch_assets.ps1)."""
    return Path(args[0]) if args else REPO / "Downloads"


def find_fbx(downloads, kit, mesh):
    """FBX for `mesh` in downloads/<kit>, or None when the pack or mesh is missing (map uses a placeholder)."""
    kit_dir = Path(downloads) / kit
    if not kit_dir.is_dir():
        return None
    hits = [p for p in kit_dir.rglob("*.fbx") if p.stem.lower() == mesh.lower() and "(unity)" not in str(p).lower()]
    return min(hits, key=lambda p: len(p.parts)) if hits else None


def meshes_by_kit():
    return maplib.meshes_by_kit(props())


def in_corridor(x, y):
    return maplib.in_corridor(x, y, CORRIDOR_LENGTH, HALF_WIDTH)


def validate():
    """Raise ValueError naming the first broken rule."""
    maplib.validate(props(), KITS, CORRIDOR_LENGTH, CORRIDOR_WIDTH, START, GATE, KILL_PLANE, DITCH, LAWN_WIDTH,
                    FLOOR_THICKNESS)


def credits_missing(credits_text):
    """(kit, mesh) pairs and kit URLs that CREDITS.md does not list."""
    return maplib.credits_missing(props(), KITS, credits_text)


def norm_name(name):
    """Asset-name key: lowercase, "-" and "_" are the same (layout `building-type-a` == asset `building_type_a`)."""
    return name.lower().replace("-", "_")


def resolve(mesh, asset_names):
    """The asset name in `asset_names` matching layout mesh `mesh`, or None."""
    by_key = {norm_name(a): a for a in asset_names}
    return by_key.get(norm_name(mesh))


def closest(mesh, asset_names):
    """Nearest asset name by normalised spelling, or None when the kit folder is empty."""
    by_key = {norm_name(a): a for a in asset_names}
    hit = difflib.get_close_matches(norm_name(mesh), list(by_key), n=1, cutoff=0.0)
    return by_key[hit[0]] if hit else None


def resolve_all(listing):
    """listing: {kit: [asset names in /Game/ThirdParty/<kit>]}.

    Returns ({(kit, mesh): asset name}, [(kit, mesh, closest asset name or None)]) for every layout mesh.
    """
    found, misses = {}, []
    for kit, names in meshes_by_kit().items():
        assets = listing.get(kit, [])
        for name in names:
            hit = resolve(name, assets)
            if hit is None:
                misses.append((kit, name, closest(name, assets)))
            else:
                found[(kit, name)] = hit
    return found, misses


def miss_report(misses):
    """Text for MISS_REPORT: one line per (kit, mesh, source) that fell back past ThirdParty."""
    lines = [f"{kit}/{mesh}: {how}" for kit, mesh, how in misses]
    return "\n".join(lines or ["no misses"]) + "\n"
