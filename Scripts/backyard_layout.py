"""Engine-free layout of the Backyard map (task 17): corridor floor, set dressing, start, gate, kill plane.

Units are metres, +X runs from start to gate, +Z is up, the corridor floor top is at Z = 0.
Scripts/build_backyard_map.py places this in UE; `make test` checks it (bounds, clear route, credits).
"""
import re
from collections import namedtuple
from pathlib import Path

import toon_spec

REPO = Path(__file__).resolve().parents[1]
MAP_PATH = "/Game/Maps/Backyard"
IMPORT_ROOT = "/Game/Imported"


def _corridor():
    """Corridor size from Core's Placement.h, so traps and the map share one source."""
    text = (REPO / "Core" / "include" / "tta" / "Placement.h").read_text(encoding="utf-8")
    values = dict(re.findall(r"constexpr double (kCorridor\w+Meters) = ([\d.]+);", text))
    try:
        return float(values["kCorridorLengthMeters"]), float(values["kCorridorWidthMeters"])
    except KeyError as missing:
        raise ValueError(f"Placement.h has no {missing}") from None


CORRIDOR_LENGTH, CORRIDOR_WIDTH = _corridor()
HALF_WIDTH = CORRIDOR_WIDTH / 2

# kit key -> (credits name, author, url, licence). `verified` = mesh names checked against the published pack.
Kit = namedtuple("Kit", "name author url licence verified")
KITS = {
    "KayKitPrototypeBits": Kit("KayKit Prototype Bits 1.0", "Kay Lousberg",
                               "https://github.com/KayKit-Game-Assets/KayKit-Prototype-Bits-1.0", "CC0 1.0", True),
    "KenneyCitySuburban": Kit("City Kit (Suburban)", "Kenney", "https://kenney.nl/assets/city-kit-suburban",
                              "CC0 1.0", False),
    "KenneyNature": Kit("Nature Kit", "Kenney", "https://kenney.nl/assets/nature-kit", "CC0 1.0", False),
}

# role: floor (walkable, blocks), marker (flat route sign on the floor, no collision), dressing (outside the
# corridor), gate (finish arch, no collision). fit: ("box", (x, y, z)) scales each axis to that size;
# ("height", h) scales uniformly to that height. Position = bottom-centre of the fitted mesh.
Prop = namedtuple("Prop", "kit mesh role x y z yaw palette fit")
Point = namedtuple("Point", "x y z")
Box = namedtuple("Box", "min_x max_x min_y max_y min_z max_z")

FLOOR_THICKNESS = 0.2
DITCH = 2.0                                  # open gap each side of the corridor, down to the kill plane
LAWN_WIDTH = 8.0
START = Point(1.0, 0.0, 0.1)                 # PlayerStart, on the patio deck
GATE = Point(CORRIDOR_LENGTH - 1.0, 0.0, 0.0)
GATE_TRIGGER_HALF = (0.5, HALF_WIDTH, 2.0)   # half extents of the finish trigger
KILL_PLANE = Box(-10.0, CORRIDOR_LENGTH + 10.0, -20.0, 20.0, -12.0, -8.0)
WORLD_KILL_Z = -500.0                        # engine KillZ, far below so ragdolls are respawned, not destroyed

# Corridor zones, start to gate: (from x, to x, palette name, reading of the route).
ZONES = (
    (0.0, 6.0, "Wood", "patio deck start"),
    (6.0, 24.0, "Grass", "front lawn"),
    (24.0, 36.0, "Yellow", "sandpit"),
    (36.0, 48.0, "Grass", "back lawn"),
    (48.0, CORRIDOR_LENGTH, "Bone", "stone patio to the gate"),
)


def palette_index(name):
    names = [n for n, _ in toon_spec.PALETTE]
    if name not in names:
        raise ValueError(f"{name!r} is not a palette colour")
    return names.index(name)


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


def _markers():
    """Bone stepping stones down the centre line every 4 m: the route reads at a glance."""
    return [Prop("KayKitPrototypeBits", "Coin_A", "marker", x, 0.0, 0.0, 0, "Bone", ("box", (0.6, 0.6, 0.03)))
            for x in range(4, int(CORRIDOR_LENGTH) - 3, 4)]


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
    return props


def _gate():
    return [Prop("KayKitPrototypeBits", "Primitive_Doorway", "gate", GATE.x, GATE.y, GATE.z, 0, "Red",
                 ("box", (1.0, CORRIDOR_WIDTH, 4.0)))]


def props():
    return _floor() + _markers() + _dressing() + _gate()


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
    used = {}
    for p in props():
        used.setdefault(p.kit, set()).add(p.mesh)
    return {kit: sorted(names) for kit, names in used.items()}


def in_corridor(x, y):
    return 0.0 <= x <= CORRIDOR_LENGTH and -HALF_WIDTH <= y <= HALF_WIDTH


def footprint(p):
    """Rough XY half-size of a prop (m): box fits are exact; height fits assume a footprint up to its height."""
    kind, size = p.fit
    if kind == "box":
        hx, hy = size[0] / 2, size[1] / 2
        return (hy, hx) if p.yaw % 180 == 90 else (hx, hy)
    return size / 2, size / 2


def validate():
    """Raise ValueError naming the first broken rule."""
    for p in props():
        if p.kit not in KITS:
            raise ValueError(f"{p.mesh}: unknown kit {p.kit}")
        palette_index(p.palette)
        hx, hy = footprint(p)
        if p.role == "dressing":
            # nothing inside the corridor: traps own it, the route stays clear
            if p.x + hx > 0 and p.x - hx < CORRIDOR_LENGTH and abs(p.y) - hy < HALF_WIDTH + DITCH:
                raise ValueError(f"dressing {p.mesh} at ({p.x}, {p.y}) intrudes on the corridor or ditch")
        elif p.role in ("marker",):
            if not in_corridor(p.x, p.y) or p.fit[1][2] > 0.05:
                raise ValueError(f"marker {p.mesh} at ({p.x}, {p.y}) must be flat and inside the corridor")
    floors = [p for p in props() if p.role == "floor" and in_corridor(p.x, p.y)]
    covered = sorted((p.x - footprint(p)[0], p.x + footprint(p)[0]) for p in floors)
    reach = 0.0
    for lo, hi in covered:
        if lo > reach + 1e-6:
            raise ValueError(f"corridor floor has a gap at x = {reach}")
        reach = max(reach, hi)
    if reach < CORRIDOR_LENGTH:
        raise ValueError(f"corridor floor stops at x = {reach}")
    if not (in_corridor(START.x, START.y) and in_corridor(GATE.x, GATE.y)):
        raise ValueError("start and gate must be inside the corridor")
    if GATE.x - START.x < 0.9 * CORRIDOR_LENGTH:
        raise ValueError("route from start to gate is shorter than 90% of the corridor")
    k = KILL_PLANE
    if not (k.min_x < 0 and k.max_x > CORRIDOR_LENGTH and k.min_y < -HALF_WIDTH - DITCH - LAWN_WIDTH
            and k.max_y > HALF_WIDTH + DITCH + LAWN_WIDTH):
        raise ValueError("kill plane does not cover the map footprint")
    if not (WORLD_KILL_Z < k.min_z < k.max_z < -FLOOR_THICKNESS - 3.0):
        raise ValueError("kill plane must sit well below the floor and above the engine KillZ")


def credits_missing(credits_text):
    """(kit, mesh) pairs and kit URLs that CREDITS.md does not list."""
    missing = []
    for kit, names in meshes_by_kit().items():
        rows = [line for line in credits_text.splitlines() if KITS[kit].url in line]
        if not rows:
            missing.append((kit, KITS[kit].url))
            continue
        for name in names:
            if not any(re.search(rf"`{re.escape(name)}`", row) for row in rows):
                missing.append((kit, name))
    return missing
