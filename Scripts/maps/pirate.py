"""Pirate Dungeon map: jail cell -> torch corridor -> sewer -> dock planks -> ship deck (Req 5-6).

Same shape as sky.py: hand-made Segment variants per section; `generate(PIRATE, seed)` picks one per section
and may mirror it. Extra kill volumes are sewer water, jail-bar and dock gaps, and the sea beside the ship."""
import sys
from collections import namedtuple
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from maplib import Box, MapDef, Segment, check_layout, generate  # noqa: E402
from maplib.grid import budget  # noqa: E402
from maplib.segment import _to_world  # noqa: E402

JAIL = (
    Segment("jail_cell_door", 12.0, 0.0, 0.0, (), ((3.0, 0.0), (9.5, 2.0))),
    Segment("jail_bars_squeeze", 13.0, 15.0, 0.0, (Box(6.0, 8.0, -4.0, -2.5, -3.0, -0.5),), ((4.0, -2.0), (8.0, 2.0), (11.0, 0.0))),
)
TORCH = (
    Segment("torch_stairs", 12.0, 0.0, 1.5, (), ((2.0, 1.5), (6.0, -1.5), (10.0, 0.0))),
    Segment("torch_bend", 11.0, -15.0, 0.5, (Box(4.0, 6.0, 2.5, 4.0, -3.0, -0.5),), ((2.0, 0.0), (8.5, -2.0))),
)
SEWER = (  # every sewer variant has water, so every layout has a kill volume beyond the shared plane
    Segment("sewer_ledges", 13.0, 30.0, -1.0,
            (Box(3.0, 10.0, -1.5, 1.5, -4.0, -0.5),),
            ((1.5, 2.5), (6.5, -2.5), (11.5, 0.0))),
    Segment("sewer_grate_hop", 12.0, -15.0, -0.5, (Box(5.0, 7.0, -4.0, 4.0, -4.0, -0.5),), ((3.0, 2.5), (9.0, -2.5))),
)
DOCK = (
    Segment("dock_planks", 12.0, 0.0, 0.0, (Box(3.0, 9.0, -4.0, -2.0, -4.0, -0.5),), ((1.5, 0.0), (10.5, 1.0))),
    Segment("dock_crates", 13.0, 15.0, 0.5, (), ((3.0, 2.0), (10.0, -2.0))),
)
SHIP = (
    Segment("ship_gangplank", 11.0, 0.0, 1.0, (), ((3.0, -2.0), (6.0, 2.0))),
    Segment("ship_deck_mast", 12.0, 0.0, 0.0, (Box(4.0, 7.0, -4.0, -2.0, -2.0, -0.5),), ((2.0, 1.0), (8.0, 2.0))),
)
SECTION_NAMES = ("jail cell", "torch corridor", "sewer", "dock planks", "ship deck")
PIRATE = MapDef("pirate", (JAIL, TORCH, SEWER, DOCK, SHIP))

START_LOCAL = (1.0, 0.0, 0.1)   # in section 1, like Backyard's START
GATE_BACK = 1.0                 # finish gate this far before the end of section 5
PLANE_MARGIN, PLANE_DEPTH = 10.0, (-12.0, -8.0)

PirateLayout = namedtuple("PirateLayout", "layout start gate kill_plane kills cells")


def _kill_plane(placed):
    xs = [v for p in placed for v in (p.entry.x, p.exit.x)]
    ys = [v for p in placed for v in (p.entry.y, p.exit.y)]
    low = min(v for p in placed for v in (p.entry.z, p.exit.z))
    return Box(min(xs) - PLANE_MARGIN, max(xs) + PLANE_MARGIN, min(ys) - PLANE_MARGIN, max(ys) + PLANE_MARGIN,
               low + PLANE_DEPTH[0], low + PLANE_DEPTH[1])


def build(seed):
    layout = generate(PIRATE, seed)
    first, last = layout.placed[0], layout.placed[-1]
    return PirateLayout(layout, _to_world(first.entry, *START_LOCAL),
                       _to_world(last.entry, last.segment.length - GATE_BACK, 0.0, 0.0),
                       _kill_plane(layout.placed), [k for p in layout.placed for k in p.kills],
                       [c for p in layout.placed for c in p.cells])


def validate(pirate):
    """Req 2-3 via check_layout, plus Req 5-6. Raises ValueError."""
    check_layout(pirate.layout)
    if len(pirate.layout.placed) != len(SECTION_NAMES):
        raise ValueError(f"pirate has {len(pirate.layout.placed)} sections, needs {len(SECTION_NAMES)}")
    for name, variants, p in zip(SECTION_NAMES, PIRATE.sections, pirate.layout.placed):
        if p.segment not in variants:
            raise ValueError(f"section {name} holds {p.segment.name}, which is not one of its variants")
    if not pirate.kills:
        raise ValueError("pirate layout has no kill volume beyond the shared kill plane")
    if not 0 < len(pirate.cells) <= budget():
        raise ValueError(f"{len(pirate.cells)} build cells, needs 1-{budget()}")
    if pirate.kill_plane.max_z >= min(p.entry.z for p in pirate.layout.placed):
        raise ValueError("kill plane is not below the path")
