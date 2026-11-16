"""Engine-free layout of the Backyard map (task 17): corridor floor, set dressing, start, gate, kill plane.

The picture: a fenced suburban back garden seen from its patio deck. The run goes down the middle (deck,
lawn, sandpit, lawn, stone patio) to a finish arch in front of the house; a water-filled trench runs down
each side; the garden beside it is furnished (barbecue, trampoline, paddling pool, shed); beyond the fences
are next-door gardens on one side and a street with houses on the other.

Units are metres, +X runs from start to gate, +Z is up, the corridor floor top is at Z = 0.
Built on Scripts/maplib (grid, roles, kill plane, credits, validate); this file keeps only the Backyard itself.
Scripts/build_backyard_map.py places this in UE; `make test` checks it (bounds, clear route, credits).
"""
import difflib
import math
from pathlib import Path

import maplib
from maplib import ROLES, WORLD_KILL_Z, Box, Kit, Point, Prop, footprint, palette_index  # noqa: F401  (map API)

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
    # Our own Blender-scripted models (models/*.py), imported by Scripts/import_mac_assets.py.
    "TTAProps": Kit("Backyard props", "Take Two: Afterlife team", "models/build_backyard_props.py", "own work", True),
    "TTACourse": Kit("Course kit", "Take Two: Afterlife team", "models/build_course_kit.py", "own work", True),
}
# Where the meshes of a kit live in /Game. Kits not listed are under THIRDPARTY_ROOT/<kit>.
KIT_ROOTS = {"TTAProps": "/Game/Art/Props", "TTACourse": "/Game/Art/Course"}


def kit_root(kit):
    """/Game folder holding the meshes of `kit`."""
    return KIT_ROOTS.get(kit, f"{THIRDPARTY_ROOT}/{kit}")


def kit_content_dir(kit):
    """The same folder on disk, under Content/."""
    return REPO / "Content" / kit_root(kit)[len("/Game/"):]


# The yard is solid ground: every slab is this deep, so each ditch is a trench with water at the bottom
# instead of a gap between floating plates. Fallers drop through the water onto the kill plane.
FLOOR_THICKNESS = 4.5
DITCH = 2.0                                  # open trench each side of the corridor, down to the kill plane
LAWN_WIDTH = 8.0                             # our yard each side, up to the fence
WATER_Z = -4.2                               # water surface in the trenches
GROUND_X = (-16.0, CORRIDOR_LENGTH + 16.0)   # the block runs past both ends of the corridor
NEIGHBOUR_WIDTH = 16.0                       # +Y: next-door gardens behind the fence
SIDEWALK, ROAD, VERGE = 3.0, 8.0, 10.0       # -Y: sidewalk, street, front gardens across the street
START = Point(1.0, 0.0, 0.1)                 # PlayerStart, on the patio deck
GATE = Point(CORRIDOR_LENGTH - 1.0, 0.0, 0.0)
GATE_TRIGGER_HALF = (0.5, HALF_WIDTH, 2.0)   # half extents of the finish trigger
KILL_PLANE = maplib.kill_plane(CORRIDOR_LENGTH, margin=24.0, half_y=44.0)

# Corridor zones, start to gate: (from x, to x, palette name, reading of the route).
ZONES = (
    (0.0, 6.0, "Wood", "patio deck start"),
    (6.0, 24.0, "Grass", "front lawn"),
    (24.0, 36.0, "Yellow", "sandpit"),
    (36.0, 48.0, "Grass", "back lawn"),
    (48.0, CORRIDOR_LENGTH, "Bone", "stone patio to the gate"),
)


FLOOR_TILE = 8.0   # longest floor piece (m): a 1 m placeholder cube or kit tile stays under scale 10


def _tiles(x0, x1):
    """Split [x0, x1] into equal, touching pieces no longer than FLOOR_TILE: (centre x, length)."""
    n = max(1, math.ceil((x1 - x0) / FLOOR_TILE - 1e-9))
    step = (x1 - x0) / n
    return [(x0 + step * (i + 0.5), step) for i in range(n)]


def _slab(x0, x1, y0, y1, colour, top=0.0, depth=FLOOR_THICKNESS, role="floor"):
    """Touching Primitive_Floor boxes filling the rectangle, top face at `top`."""
    return [Prop("KayKitPrototypeBits", "Primitive_Floor", role, x, y, top - depth, 0, colour,
                 ("box", (lx, ly, depth)))
            for x, lx in _tiles(x0, x1) for y, ly in _tiles(y0, y1)]


def _floor():
    edge = HALF_WIDTH + DITCH
    fence = edge + LAWN_WIDTH
    gx0, gx1 = GROUND_X
    props = []
    for x0, x1, colour, _ in ZONES:
        props += _slab(x0, x1, -HALF_WIDTH, HALF_WIDTH, colour)
    # End caps close the trenches: lawn behind the start, the patio runs on to the house past the gate.
    props += _slab(gx0, 0.0, -edge, edge, "Grass")
    props += _slab(CORRIDOR_LENGTH, gx1, -edge, edge, "Bone")
    for side in (-1, 1):
        y0, y1 = sorted((side * edge, side * fence))
        props += _slab(gx0, gx1, y0, y1, "Grass")
    props += _slab(gx0, gx1, fence, fence + NEIGHBOUR_WIDTH, "Grass")
    y = -fence
    for width, colour in ((SIDEWALK, "Bone"), (ROAD, "Ink"), (VERGE, "Grass")):
        props += _slab(gx0, gx1, y - width, y, colour)
        y -= width
    return props


def _water():
    """Water in both trenches (no collision): the fall reads as a splash, not a hole in the world."""
    props = []
    for side in (-1, 1):
        y0, y1 = sorted((side * HALF_WIDTH, side * (HALF_WIDTH + DITCH)))
        props += _slab(0.0, CORRIDOR_LENGTH, y0, y1, "Sky", top=WATER_Z, depth=0.2, role="backdrop")
    return props


ROUTE_TAG = "TTARoute"  # visual stepping stones; bots race the finish gate and do not follow these


def _markers():
    """Bone stepping stones down the centre line every 4 m: the route reads at a glance."""
    return [Prop("KayKitPrototypeBits", "Coin_A", "marker", x, 0.0, 0.0, 0, "Bone", ("box", (0.6, 0.6, 0.05)))
            for x in range(4, int(CORRIDOR_LENGTH) - 3, 4)]


# Alternate branches: (leave marker x, rejoin marker x, side). Each bows out to |y| = 2.5 and back.
SHORTCUTS = ((16, 24, 1), (40, 48, -1))


def _shortcuts():
    """Flat stones (no TTARoute tag) for two optional side branches between route markers."""
    props = []
    for x0, x1, side in SHORTCUTS:
        for dx, y in ((1.5, 1.5), (3.0, 2.5), (5.0, 2.5), (6.5, 1.5)):
            props.append(Prop("KayKitPrototypeBits", "Coin_A", "shortcut", x0 + dx, side * y, 0.0, 0, "Sky",
                              ("box", (0.6, 0.6, 0.05))))
        assert x0 + 6.5 < x1
    return props


FENCE_STEP = 2.1   # one `fence` panel fitted to 1.2 m tall is 2.1 m long: panels touch


def _fences(fence):
    """Unbroken garden fence down both long edges of the yard and across both ends."""
    gx0, gx1 = GROUND_X
    props = []
    n = math.ceil((gx1 - gx0) / FENCE_STEP)
    for i in range(n):
        x = gx0 + (gx1 - gx0) * (i + 0.5) / n
        for side in (-1, 1):
            props.append(Prop("KenneyCitySuburban", "fence", "dressing", x, side * (fence - 0.3), 0.0, 0,
                              "Bone", ("height", 1.2)))
    m = math.ceil(2 * (fence - 0.3) / FENCE_STEP)
    for i in range(m):
        y = -(fence - 0.3) + 2 * (fence - 0.3) * (i + 0.5) / m
        for x in (gx0 + 0.3, gx1 - 0.3):
            props.append(Prop("KenneyCitySuburban", "fence", "dressing", x, y, 0.0, 90, "Bone", ("height", 1.2)))
    return props


def _height(v):
    return ("height", v)


def _box(*v):
    return ("box", v)


def _yard(edge):
    """Our garden, start to gate. +Y: barbecue, trampoline, paddling pool, tea table. -Y: tool corner, flower
    border, rockery, the shed. Rows are (x, d, kit, mesh, colour, fit, yaw); d is the distance in from the
    trench edge, its sign picks the side."""
    P, K, N, S = "TTAProps", "KayKitPrototypeBits", "KenneyNature", "KenneyCitySuburban"
    h, b = _height, _box
    rows = (
        # +Y side, by the deck: barbecue corner
        (3.0, 3.5, P, "SM_Backyard_PicnicTable", "Wood", b(1.8, 1.5, 0.773), 0),
        (6.5, 2.0, P, "SM_Backyard_Grill", "Ink", b(0.97, 0.64, 0.997), 90),
        (1.0, 6.2, S, "planter", "Wood", h(0.8), 0),
        (9.5, 6.0, S, "tree-small", "Grass", h(4.0), 0),
        # front lawn: trampoline, a gnome keeping watch
        (12.5, 1.2, N, "plant_bushLarge", "Teal", h(1.0), 0),
        (14.0, 1.0, N, "flower_redA", "Red", h(0.5), 0),
        (18.0, 4.0, P, "SM_Backyard_Trampoline", "Blue", b(3.0, 3.0, 0.889), 0),
        (22.5, 1.0, P, "SM_Backyard_Gnome", "Red", h(0.7), 180),
        (23.5, 6.5, N, "plant_bushLarge", "Teal", h(1.2), 0),
        # sandpit: paddling pool and toy blocks
        (29.0, 3.0, P, "SM_Backyard_KiddiePool", "Sky", b(1.8, 1.8, 0.36), 0),
        (26.5, 5.5, K, "Barrel_A", "Blue", h(0.8), 0),
        (31.8, 5.2, K, "Box_A", "Red", h(0.8), 0),
        (32.8, 6.2, K, "Box_B", "Purple", h(0.8), 20),
        (33.5, 1.0, N, "flower_yellowA", "Yellow", h(0.5), 0),
        (36.0, 5.8, S, "tree-large", "Grass", h(6.5), 0),
        # back lawn: clipped hedge
        (39.0, 1.2, N, "plant_bushLarge", "Teal", h(1.0), 0),
        (41.0, 1.2, N, "plant_bushLarge", "Teal", h(1.0), 0),
        (43.0, 1.2, N, "plant_bushLarge", "Teal", h(1.0), 0),
        (45.5, 1.0, N, "flower_redA", "Pink", h(0.5), 0),
        # stone patio: tea table and planters up to the gate
        (51.0, 3.0, K, "table_medium", "Wood", h(0.75), 0),
        (47.5, 6.0, S, "planter", "Wood", h(0.8), 45),
        (54.5, 1.5, S, "planter", "Wood", h(0.8), 0),
        (57.5, 1.5, S, "planter", "Wood", h(0.8), 0),
        (56.0, 5.5, S, "tree-small", "Grass", h(3.5), 0),
        # -Y side, by the deck: tool corner
        (2.0, -3.0, K, "Pallet_Small", "Wood", h(0.8), 0),
        (3.6, -2.2, K, "Barrel_A", "Orange", h(0.8), 0),
        (0.8, -5.5, K, "Box_A", "Wood", h(0.8), 15),
        (9.0, -5.5, S, "tree-large", "Grass", h(6.0), 0),
        # front lawn: gnome and a flower border along the trench
        (12.0, -1.0, P, "SM_Backyard_Gnome", "Blue", h(0.7), 0),
        (13.5, -1.0, N, "flower_yellowA", "Yellow", h(0.5), 0),
        (15.0, -1.0, N, "flower_redA", "Red", h(0.5), 0),
        (16.5, -1.0, N, "flower_yellowA", "Orange", h(0.5), 0),
        (18.0, -1.0, N, "flower_redA", "Pink", h(0.5), 0),
        (20.5, -1.4, N, "rock_smallA", "Ink", h(0.4), 0),
        (22.0, -5.0, N, "plant_bushLarge", "Teal", h(1.4), 0),
        # sandpit side: rockery
        (26.0, -1.5, N, "rock_smallA", "Ink", h(0.5), 0),
        (27.5, -2.6, N, "rock_smallA", "Ink", h(0.8), 40),
        (29.5, -1.3, N, "plant_bushLarge", "Teal", h(0.9), 0),
        (31.5, -1.0, N, "flower_yellowA", "Yellow", h(0.5), 0),
        (33.0, -5.5, S, "tree-small", "Grass", h(4.0), 0),
        # back lawn: the shed and its clutter
        (41.5, -4.5, P, "SM_Backyard_Shed", "Red", b(2.5, 2.0, 2.3), 0),
        (38.5, -3.5, K, "Pallet_Small", "Wood", h(0.8), 30),
        (44.5, -3.0, K, "Barrel_A", "Blue", h(0.8), 0),
        (45.5, -4.2, K, "Box_B", "Wood", h(0.8), 0),
        (37.0, -1.0, P, "SM_Backyard_Gnome", "Yellow", h(0.7), 90),
        # stone patio
        (49.0, -5.5, S, "tree-small", "Grass", h(3.5), 0),
        (52.0, -1.5, S, "planter", "Wood", h(0.8), 0),
        (55.0, -1.5, S, "planter", "Wood", h(0.8), 0),
        (58.0, -1.2, N, "plant_bushLarge", "Teal", h(1.0), 0),
        (53.5, -5.0, N, "rock_smallA", "Ink", h(0.5), 0),
    )
    return [Prop(kit, mesh, "dressing", x, (edge + abs(d)) * (1 if d > 0 else -1), 0.0, yaw, colour, fit)
            for x, d, kit, mesh, colour, fit, yaw in rows]


def _neighbourhood(fence):
    """Houses all round: the one the gate leads into, the one behind the start, next-door gardens on +Y and
    a street with houses across it on -Y."""
    S = "KenneyCitySuburban"
    props = [
        Prop(S, "building-type-a", "dressing", CORRIDOR_LENGTH + 7.0, 0.0, 0.0, 180, "Sky", ("height", 7.0)),
        Prop(S, "building-type-c", "dressing", -8.5, 0.0, 0.0, 0, "Orange", ("height", 7.0)),
    ]
    next_door = (("building-type-b", "Yellow"), ("building-type-d", "Pink"), ("building-type-e", "Teal"),
                 ("building-type-f", "Purple"), ("building-type-b", "Orange"))
    for i, (mesh, colour) in enumerate(next_door):
        x = -6.0 + 18.0 * i
        props.append(Prop(S, mesh, "dressing", x, fence + 9.5, 0.0, 270, colour, ("height", 7.0)))
        props.append(Prop(S, "tree-large" if i % 2 else "tree-small", "dressing", x + 9.0, fence + 5.0, 0.0, 0,
                          "Grass", ("height", 5.5 if i % 2 else 4.0)))
    street = -fence - SIDEWALK - ROAD
    across = (("building-type-d", "Sky"), ("building-type-f", "Yellow"), ("building-type-a", "Pink"),
              ("building-type-e", "Orange"), ("building-type-c", "Teal"))
    for i, (mesh, colour) in enumerate(across):
        x = 2.0 + 17.0 * i
        props.append(Prop(S, mesh, "dressing", x, street - 6.0, 0.0, 90, colour, ("height", 7.0)))
    for x in range(-10, int(CORRIDOR_LENGTH) + 12, 12):                       # street trees on the sidewalk
        props.append(Prop(S, "tree-small", "dressing", float(x), -fence - 1.5, 0.0, 0, "Grass", ("height", 4.5)))
    return props


def _dressing():
    edge = HALF_WIDTH + DITCH                     # inner edge of each side lawn
    fence = edge + LAWN_WIDTH
    return _fences(fence) + _yard(edge) + _neighbourhood(fence)


def _gate():
    # SM_Course_GateFinish is 3 m wide along its own X: turned to span the corridor and scaled to its width.
    return [Prop("TTACourse", "SM_Course_GateFinish", "gate", GATE.x, GATE.y, GATE.z, 90, "Red",
                 ("width", CORRIDOR_WIDTH))]


def props():
    return _floor() + _water() + _markers() + _shortcuts() + _dressing() + _gate()


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
    """listing: {kit: [asset names in kit_root(kit)]}.

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
