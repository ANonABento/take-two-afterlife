"""House map: lawn -> shed/drainpipe -> bathroom window -> counter/sink -> bathtub (Req 5-6).

Each section has hand-made Segment variants; `generate(HOUSE, seed)` picks one per section and may mirror it.
"""
import sys
from collections import namedtuple
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from maplib import Box, MapDef, Segment, check_layout, generate  # noqa: E402
from maplib.grid import budget  # noqa: E402
from maplib.segment import _to_world  # noqa: E402

LAWN = (
    Segment("lawn_hedge", 12.0, 0.0, 0.0, (), ((4.0, 0.0), (6.0, -2.0), (8.0, 2.0))),
    Segment("lawn_sprinkler", 13.0, 15.0, 0.0, (), ((5.0, 1.5), (7.5, -1.5), (10.0, 0.0))),
)
SHED = (
    Segment("shed_roof", 12.0, 0.0, 1.5, (Box(5.0, 8.0, -4.0, 4.0, -3.0, -1.0),), ((2.0, 0.0), (9.0, 2.0))),
    Segment("drainpipe_climb", 14.0, 30.0, 1.5, (), ((3.0, -2.5), (7.0, 0.0), (11.0, 2.5))),
)
WINDOW = (
    Segment("window_ledge", 11.0, -15.0, 1.0, (Box(3.0, 8.0, 2.0, 4.0, -4.0, -1.0),), ((2.0, -1.0), (9.0, 1.0))),
    Segment("window_drop", 12.0, 0.0, -1.5, (), ((4.0, 0.0), (8.0, -3.0))),
)
COUNTER = (
    Segment("counter_sink", 11.0, 15.0, 0.0, (Box(4.0, 7.0, -1.5, 1.5, -1.0, -0.2),), ((2.0, 2.0), (9.0, -2.0))),
    Segment("counter_soap_slide", 13.0, -30.0, -1.0, (), ((3.0, 0.0), (6.5, 2.5), (10.0, -2.5))),
)
TUB = (  # every bathtub variant has water, so every layout has a kill volume beyond the shared plane
    Segment("tub_rim", 12.0, 0.0, 0.0, (Box(2.0, 10.0, -4.0, -1.0, -2.0, -0.5),), ((3.0, 2.0), (6.0, 2.0))),
    Segment("tub_duck_hop", 13.0, 0.0, 0.5, (Box(3.0, 11.0, -4.0, 4.0, -2.5, -1.0),), ((1.5, 0.0), (11.5, 0.0))),
)
SECTION_NAMES = ("lawn", "shed/drainpipe", "bathroom window", "counter/sink", "bathtub")
HOUSE = MapDef("house", (LAWN, SHED, WINDOW, COUNTER, TUB))

START_LOCAL = (1.0, 0.0, 0.1)   # in section 1, like Backyard's START
GATE_BACK = 1.0                 # finish gate this far before the end of section 5
PLANE_MARGIN, PLANE_DEPTH = 10.0, (-12.0, -8.0)

HouseLayout = namedtuple("HouseLayout", "layout start gate kill_plane kills cells")


def _kill_plane(placed):
    xs = [v for p in placed for v in (p.entry.x, p.exit.x)]
    ys = [v for p in placed for v in (p.entry.y, p.exit.y)]
    low = min(v for p in placed for v in (p.entry.z, p.exit.z))
    return Box(min(xs) - PLANE_MARGIN, max(xs) + PLANE_MARGIN, min(ys) - PLANE_MARGIN, max(ys) + PLANE_MARGIN,
               low + PLANE_DEPTH[0], low + PLANE_DEPTH[1])


def build(seed):
    layout = generate(HOUSE, seed)
    first, last = layout.placed[0], layout.placed[-1]
    return HouseLayout(layout, _to_world(first.entry, *START_LOCAL),
                       _to_world(last.entry, last.segment.length - GATE_BACK, 0.0, 0.0),
                       _kill_plane(layout.placed), [k for p in layout.placed for k in p.kills],
                       [c for p in layout.placed for c in p.cells])


def validate(house):
    """Req 2-3 via check_layout, plus Req 5-6. Raises ValueError."""
    check_layout(house.layout)
    if len(house.layout.placed) != len(SECTION_NAMES):
        raise ValueError(f"house has {len(house.layout.placed)} sections, needs {len(SECTION_NAMES)}")
    for name, variants, p in zip(SECTION_NAMES, HOUSE.sections, house.layout.placed):
        if p.segment not in variants:
            raise ValueError(f"section {name} holds {p.segment.name}, which is not one of its variants")
    if not house.kills:
        raise ValueError("house layout has no kill volume beyond the shared kill plane")
    if not 0 < len(house.cells) <= budget():
        raise ValueError(f"{len(house.cells)} build cells, needs 1-{budget()}")
    if house.kill_plane.max_z >= min(p.entry.z for p in house.layout.placed):
        raise ValueError("kill plane is not below the path")
