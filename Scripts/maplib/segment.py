"""Segments: hand-made path pieces that chain entry-to-exit into one route, picked per section by a seed.

A segment is authored in its own frame: entry at the origin heading +X, floor top at Z = 0, path |y| <= width / 2.
It ends with an optional height step and turn; the chain carries each exit pose into the next segment's entry.
"""
import math
import random
from collections import namedtuple

from maplib.grid import corridor, grid_meters
from maplib.roles import Box

Pose = namedtuple("Pose", "x y z heading")     # metres, heading in degrees (0 = +X, counter-clockwise)
Segment = namedtuple("Segment", "name length turn rise kills cells")
# turn: degrees at the exit, multiple of TURN_STEP. rise: Z change at the exit (<= MAX_STEP_UP up, any drop).
# kills: local Boxes. cells: local (x, y) build cell centres on the grid.
Placed = namedtuple("Placed", "segment mirrored entry exit kills cells")
Layout = namedtuple("Layout", "map seed placed length")
MapDef = namedtuple("MapDef", "name sections")  # sections: tuple of tuples of Segment variants, in route order

ORIGIN = Pose(0.0, 0.0, 0.0, 0.0)
TURN_STEP = 15.0
MAX_STEP_UP = 1.5
LENGTH_RANGE = (54.0, 75.0)
POS_TOL = 0.01
HEADING_TOL = 0.5


def _to_world(pose, x, y, z):
    h = math.radians(pose.heading)
    return (pose.x + x * math.cos(h) - y * math.sin(h), pose.y + x * math.sin(h) + y * math.cos(h), pose.z + z)


def place(segment, entry, mirrored=False):
    """Put a segment at an entry pose; mirroring flips its turn and its local Y."""
    sy = -1.0 if mirrored else 1.0
    x, y, _ = _to_world(entry, segment.length, 0.0, 0.0)
    exit_ = Pose(x, y, entry.z + segment.rise, (entry.heading + sy * segment.turn) % 360.0)
    kills = []
    for b in segment.kills:
        corners = [_to_world(entry, cx, sy * cy, 0.0) for cx in (b.min_x, b.max_x) for cy in (b.min_y, b.max_y)]
        xs, ys = [c[0] for c in corners], [c[1] for c in corners]
        kills.append(Box(min(xs), max(xs), min(ys), max(ys), entry.z + b.min_z, entry.z + b.max_z))
    cells = [_to_world(entry, cx, sy * cy, 0.0) for cx, cy in segment.cells]
    return Placed(segment, mirrored, entry, exit_, kills, cells)


def generate(mapdef, seed, start=ORIGIN):
    """Pick one variant per section (and maybe mirror it) from the seed, then chain them. Same inputs, same layout."""
    rng = random.Random(f"{mapdef.name}:{seed}")
    placed, pose = [], start
    for variants in mapdef.sections:
        p = place(variants[rng.randrange(len(variants))], pose, mirrored=rng.random() < 0.5)
        placed.append(p)
        pose = p.exit
    return Layout(mapdef.name, seed, placed, sum(p.segment.length for p in placed))


def _heading_diff(a, b):
    return abs((a - b + 180.0) % 360.0 - 180.0)


def check_segment(s):
    """Raise ValueError if a segment breaks the authoring rules."""
    grid, (_, width) = grid_meters(), corridor()
    if s.length <= 0:
        raise ValueError(f"segment {s.name}: length must be positive")
    if abs(s.turn / TURN_STEP - round(s.turn / TURN_STEP)) > 1e-9:
        raise ValueError(f"segment {s.name}: turn {s.turn} is not a multiple of {TURN_STEP} degrees")
    if s.rise > MAX_STEP_UP:
        raise ValueError(f"segment {s.name}: step up {s.rise} m is over {MAX_STEP_UP} m")
    for cx, cy in s.cells:
        if not (0 <= cx <= s.length and abs(cy) <= width / 2):
            raise ValueError(f"segment {s.name}: build cell ({cx}, {cy}) is off the path")
        if any(abs(v / grid - round(v / grid)) > 1e-9 for v in (cx, cy)):
            raise ValueError(f"segment {s.name}: build cell ({cx}, {cy}) is off the {grid} m grid")


def check_layout(layout):
    """Req 2-3: segments chain (<= 1 cm, <= 0.5 deg), each is valid, total length in range. Raises ValueError."""
    for p in layout.placed:
        check_segment(p.segment)
    for prev, nxt in zip(layout.placed, layout.placed[1:]):
        gap = math.dist(prev.exit[:3], nxt.entry[:3])
        if gap > POS_TOL or _heading_diff(prev.exit.heading, nxt.entry.heading) > HEADING_TOL:
            raise ValueError(f"{nxt.segment.name} does not start where {prev.segment.name} ends "
                             f"(gap {gap:.3f} m, heading {prev.exit.heading} vs {nxt.entry.heading})")
    lo, hi = LENGTH_RANGE
    if not lo <= layout.length <= hi:
        raise ValueError(f"path length {layout.length} m is outside {lo}-{hi} m")
