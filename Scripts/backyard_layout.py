"""Engine-free layout of the Backyard map (task 17): corridor floor, set dressing, start, gate, kill plane.

The picture: a back garden lifted into the sky. The run is one floating strip (deck, lawn, sandpit, lawn,
stone patio) to a finish arch; leave it and you fall into the clouds. Round it, out of reach, drift chunks
of the same neighbourhood: houses on squares of lawn, fences, trees.

Units are metres, +X runs from start to gate, +Z is up, the corridor floor top is at Z = 0.
Built on Scripts/maplib (grid, roles, kill plane, credits, validate); this file keeps only the Backyard itself.
Scripts/build_backyard_map.py places this in UE; `make test` checks it (bounds, clear route, credits).
"""
import difflib
import math
import random
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
    "TTAHeaven": Kit("Heaven set", "Take Two: Afterlife team", "models/build_heaven.py", "own work", True),
}
# Where the meshes of a kit live in /Game. Kits not listed are under THIRDPARTY_ROOT/<kit>.
KIT_ROOTS = {"TTAProps": "/Game/Art/Props", "TTACourse": "/Game/Art/Course", "TTAHeaven": "/Game/Art/Heaven"}


def kit_root(kit):
    """/Game folder holding the meshes of `kit`."""
    return KIT_ROOTS.get(kit, f"{THIRDPARTY_ROOT}/{kit}")


def kit_content_dir(kit):
    """The same folder on disk, under Content/."""
    return REPO / "Content" / kit_root(kit)[len("/Game/"):]


# The Backyard is one floating strip of garden. Round it, out of reach, drift chunks of the same neighbourhood
# (a house on its square of lawn, a fence, a tree) over a cloud layer. Anyone who leaves the strip falls into
# the clouds and onto the kill plane.
FLOOR_THICKNESS = 3.0                        # the strip is a thick chunk of ground, not a plate
EARTH = 1.5                                  # darker earth under every chunk of ground
DITCH = 6.0                                  # clear air each side of the path: nothing there to land on
LAWN_WIDTH = 0.0
PAD = 8.0                                    # start and finish pads beyond the corridor ends
PAD_HALF = 7.0                               # pads are wider than the path: room for the shed and the table
STRIP_X = (-PAD, CORRIDOR_LENGTH + PAD)
CLOUD_Z = -10.0                              # cloud layer under the strip; the kill plane is inside it
START = Point(1.0, 0.0, 0.1)                 # PlayerStart, on the patio deck
GATE = Point(CORRIDOR_LENGTH - 1.0, 0.0, 0.0)
GATE_TRIGGER_HALF = (0.5, HALF_WIDTH, 2.0)   # half extents of the finish trigger
KILL_PLANE = maplib.kill_plane(CORRIDOR_LENGTH, margin=60.0, half_y=80.0, min_z=-14.0, max_z=-9.5)
HEAVEN_SEAT = (0.0, 140.0, 70.0)             # ATTAGameMode::OpenHeaven spawns the couch and TV here: keep it clear
SEED = 7                                     # same islands every rebuild

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
    """Touching boxes filling the rectangle, top face at `top`."""
    return [Prop("KayKitPrototypeBits", "Primitive_Floor", role, x, y, top - depth, 0, colour,
                 ("box", (lx, ly, depth)))
            for x, lx in _tiles(x0, x1) for y, ly in _tiles(y0, y1)]


SCENERY_TILE = 10.0   # a 1 m cube may be scaled to 10 at most (layout_spawn.MAX_SCALE)


def _ground(x0, x1, y0, y1, colour, top, depth):
    """Unwalked ground (no collision), in as few boxes as the scale limit allows."""
    def cut(a, b):
        n = max(1, math.ceil((b - a) / SCENERY_TILE - 1e-9))
        return [(a + (b - a) * (i + 0.5) / n, (b - a) / n) for i in range(n)]
    return [Prop("KayKitPrototypeBits", "Primitive_Floor", "backdrop", round(x, 2), round(y, 2), top - depth, 0, colour,
                 ("box", (round(lx, 2), round(ly, 2), depth))) for x, lx in cut(x0, x1) for y, ly in cut(y0, y1)]


def _chunk(x0, x1, y0, y1, colour, top, depth):
    """A floating piece of ground: the slab and a smaller block of earth hanging under it."""
    inset = min(1.0, (x1 - x0) / 6, (y1 - y0) / 6)
    return (_ground(x0, x1, y0, y1, colour, top, depth)
            + _ground(x0 + inset, x1 - inset, y0 + inset, y1 - inset, "Wood", top - depth, EARTH))


def _floor():
    props = []
    for x0, x1, colour, _ in ZONES:
        props += _slab(x0, x1, -HALF_WIDTH, HALF_WIDTH, colour)
    props += _slab(STRIP_X[0], 0.0, -PAD_HALF, PAD_HALF, "Wood")
    props += _slab(CORRIDOR_LENGTH, STRIP_X[1], -PAD_HALF, PAD_HALF, "Bone")
    props += _ground(STRIP_X[0] + 1.0, STRIP_X[1] - 1.0, -HALF_WIDTH + 1.0, HALF_WIDTH - 1.0, "Wood",
                     -FLOOR_THICKNESS, EARTH)
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
# Broken picket fence along the strip edges, seen and not felt: (from x, to x, side). Open edges stay open.
FENCE_RUNS = ((4, 14, 1), (20, 30, 1), (38, 46, 1), (52, 58, 1), (8, 18, -1), (26, 34, -1), (42, 52, -1))


def _fence_run(x0, x1, y, z=0.0):
    n = max(1, round((x1 - x0) / FENCE_STEP))
    return [Prop("KenneyCitySuburban", "fence", "scenery", x0 + FENCE_STEP * (i + 0.5), y, z, 0, "Bone",
                 ("height", 1.2)) for i in range(n)]


def _height(v):
    return ("height", v)


def _box(*v):
    return ("box", v)


def _pads():
    """Furniture on the two pads only: the course itself stays bare for the traps."""
    P, S, N = "TTAProps", "KenneyCitySuburban", "KenneyNature"
    h, b = _height, _box
    end = CORRIDOR_LENGTH
    rows = (
        (-5.0, 4.5, P, "SM_Backyard_Shed", "Red", b(2.5, 2.0, 2.3), 0),
        (-2.5, -5.2, P, "SM_Backyard_Grill", "Ink", b(0.97, 0.64, 0.997), 90),
        (-6.0, -4.5, S, "tree-small", "Grass", h(4.0), 0),
        (-1.2, 5.8, S, "planter", "Wood", h(0.8), 0),
        (end + 4.5, -4.5, P, "SM_Backyard_PicnicTable", "Wood", b(1.8, 1.5, 0.773), 0),
        (end + 2.0, 5.6, P, "SM_Backyard_Gnome", "Red", h(0.7), 180),
        (end + 6.0, 4.5, S, "tree-large", "Grass", h(6.0), 0),
        (end + 1.5, -5.8, S, "planter", "Wood", h(0.8), 0),
        (end + 6.5, -1.0, N, "plant_bushLarge", "Teal", h(1.2), 0),
    )
    props = [Prop(kit, mesh, "dressing", x, y, 0.0, yaw, colour, fit) for x, y, kit, mesh, colour, fit, yaw in rows]
    for x0, x1, side in FENCE_RUNS:
        props += _fence_run(x0, x1, side * (HALF_WIDTH - 0.15))
    return props


HOUSES = tuple(f"building-type-{c}" for c in "abcdefghijklmnopqrstu")
HOUSE_COLOURS = ("Sky", "Orange", "Yellow", "Pink", "Teal", "Purple", "Red", "Blue")
YARD_TOYS = (("SM_Backyard_Trampoline", "Blue", ("box", (3.0, 3.0, 0.889))),
             ("SM_Backyard_KiddiePool", "Sky", ("box", (1.8, 1.8, 0.36))),
             ("SM_Backyard_PicnicTable", "Wood", ("box", (1.8, 1.5, 0.773))))
NEAR_GAP = 14.0   # nearest island edge to the strip (m): nearly three running jumps


def strip_distance(x, y):
    """Distance in the ground plane from (x, y) to the strip and its pads."""
    dx = max(STRIP_X[0] - x, 0.0, x - STRIP_X[1])
    dy = max(abs(y) - PAD_HALF, 0.0)
    return math.hypot(dx, dy)


def island_sites():
    """(x, y, z of the lawn top, size, far) for every island. Most sit on +Y and past both ends: that is where the
    runner camera and the TV cameras (on -Y, looking across the strip) look. -Y gets only a few far ones."""
    rng = random.Random(SEED)
    sites = []

    def room(x, y, size):
        if math.hypot(x - HEAVEN_SEAT[0], y - HEAVEN_SEAT[1]) < 45.0:
            return False
        return all(math.hypot(x - sx, y - sy) > 0.8 * (size + ss) for sx, sy, _, ss, _ in sites)

    def scatter(count, far, minus_y):
        placed = 0
        for _ in range(4000):
            if placed == count:
                return
            size = rng.uniform(15.0, 20.0) if far else rng.uniform(10.0, 14.0)
            gap = rng.uniform(45.0, 110.0) if far else rng.uniform(NEAR_GAP, 30.0)
            if far:
                x = rng.uniform(STRIP_X[0] - 60.0, STRIP_X[1] + 90.0)
            else:
                x = rng.uniform(STRIP_X[0] - 20.0, STRIP_X[1] + 30.0)
            y = (-1 if minus_y else 1) * (PAD_HALF + gap + size / 2)
            if not minus_y and rng.random() < 0.3:      # some sit dead ahead, past the gate
                x, y = STRIP_X[1] + gap + size / 2, rng.uniform(-25.0, 40.0)
            if strip_distance(x, y) - size * 0.71 < (40.0 if far else NEAR_GAP) or not room(x, y, size):
                continue
            z = rng.uniform(-10.0, 22.0) if far else rng.uniform(-6.0, 8.0)
            sites.append((round(x, 1), round(y, 1), round(z, 1), round(size, 1), far))
            placed += 1
        if placed < count:
            raise ValueError("no room left for the islands")

    scatter(8, False, False)
    scatter(13, True, False)
    scatter(3, True, True)
    return sites


def _island(index, x, y, z, size, far, rng):
    """One drifting chunk of the neighbourhood: lawn, a house, a tree, a bit of fence, sometimes a toy."""
    S, P = "KenneyCitySuburban", "TTAProps"
    half = size / 2
    props = _chunk(x - half, x + half, y - half, y + half, "Grass", top=z, depth=2.5)
    tall = 9.0 if far else 7.0
    props.append(Prop(S, HOUSES[index % len(HOUSES)], "scenery", x - half * 0.2, y + half * 0.15, z,
                      rng.choice((0, 90, 180, 270)), HOUSE_COLOURS[index % len(HOUSE_COLOURS)], ("height", tall)))
    corner = rng.choice(((1, -1), (1, 1), (-1, -1)))
    props.append(Prop(S, "tree-large" if index % 2 else "tree-small", "scenery", x + corner[0] * half * 0.7,
                      y + corner[1] * half * 0.7, z, 0, "Grass", ("height", 6.5 if index % 2 else 4.5)))
    if not far:
        props += _fence_run(x - half + 0.5, x + half - 0.5, y - half + 0.3, z)
        if index % 2 == 0:
            mesh, colour, fit = YARD_TOYS[(index // 2) % len(YARD_TOYS)]
            props.append(Prop(P, mesh, "scenery", x + half * 0.5, y - half * 0.45, z, 0, colour, fit))
    return props


def _islands():
    rng = random.Random(SEED + 1)
    props = []
    for index, (x, y, z, size, far) in enumerate(island_sites()):
        props += _island(index, x, y, z, size, far, rng)
    return props


DEBRIS = (("fence", "Bone", ("height", 1.2)), ("planter", "Wood", ("height", 0.8)),
          ("path-stones-short", "Bone", ("height", 0.3)), ("tree-small", "Grass", ("height", 3.0)))


def _debris():
    """Bits torn loose, hanging in the air between the strip and the near islands."""
    rng = random.Random(SEED + 2)
    props = []
    for i in range(12):
        x = rng.uniform(STRIP_X[0], STRIP_X[1] + 10.0)
        y = rng.choice((1, 1, 1, -1)) * rng.uniform(PAD_HALF + 5.0, PAD_HALF + 12.0)
        mesh, colour, fit = DEBRIS[i % len(DEBRIS)]
        props.append(Prop("KenneyCitySuburban", mesh, "scenery", round(x, 1), round(y, 1),
                          round(rng.uniform(-3.0, 5.0), 1), rng.choice((0, 30, 60, 90, 135)), colour, fit))
    return props


# Cloud meshes and the widest each may be fitted (m): ten times its own width, the scale limit.
CLOUDS = (("SM_Heaven_Cloud_A", 19.0), ("SM_Heaven_Cloud_B", 34.0), ("SM_Heaven_Cloud_C", 49.0))


def _cloud(i, x, y, z, yaw, share):
    mesh, widest = CLOUDS[i % len(CLOUDS)]
    return Prop("TTAHeaven", mesh, "scenery", round(x, 1), round(y, 1), round(z, 1), yaw, "Bone",
                ("width", round(widest * share, 1)))


def _clouds():
    """A loose cloud floor under everything (fallers vanish into it) and a few big ones out among the far islands."""
    rng = random.Random(SEED + 3)
    props = []
    for i, x in enumerate(range(-70, 171, 20)):
        for j, y in enumerate(range(-72, 121, 20)):
            props.append(_cloud(i + j, x + rng.uniform(-5.0, 5.0), y + rng.uniform(-5.0, 5.0),
                                CLOUD_Z + rng.uniform(-2.5, 0.0), rng.choice((0, 90, 180)), rng.uniform(0.75, 0.98)))
    for i in range(10):
        x, y = rng.uniform(-80.0, 200.0), rng.choice((1, 1, -1)) * rng.uniform(70.0, 150.0)
        if math.hypot(x - HEAVEN_SEAT[0], y - HEAVEN_SEAT[1]) >= 60.0:
            props.append(_cloud(1 + i % 2, x, y, rng.uniform(5.0, 30.0), rng.choice((0, 90)), 0.98))
    return props


def _dressing():
    return _pads() + _islands() + _debris() + _clouds()


def _gate():
    # SM_Course_GateFinish is 3 m wide along its own X: turned to span the corridor and scaled to its width.
    return [Prop("TTACourse", "SM_Course_GateFinish", "gate", GATE.x, GATE.y, GATE.z, 90, "Red",
                 ("width", CORRIDOR_WIDTH))]


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
