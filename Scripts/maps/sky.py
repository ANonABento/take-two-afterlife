"""Sky Construction map: crane jib -> scaffolding -> girders -> hook ride -> sandpit/dumpster (Req 5-6).

Same shape as house.py: hand-made Segment variants per section; `generate(SKY, seed)` picks one per section
and may mirror it. Extra kill volumes are the fall gaps between planks, girders and under the hook.
"""
import sys
from collections import namedtuple
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from maplib import Box, MapDef, Segment, check_layout, generate  # noqa: E402
from maplib.grid import budget  # noqa: E402
from maplib.segment import _to_world  # noqa: E402

JIB = (
    Segment("jib_walk", 12.0, 0.0, 0.0, (Box(6.0, 8.0, -4.0, 4.0, -6.0, -0.5),), ((3.0, 0.0), (9.5, 2.0))),
    Segment("jib_counterweight", 13.0, 15.0, -1.0, (), ((4.0, -2.0), (8.0, 2.0), (11.0, 0.0))),
)
SCAFFOLD = (
    Segment("scaffold_ladder", 12.0, 0.0, 1.5, (), ((2.0, 1.5), (6.0, -1.5), (10.0, 0.0))),
    Segment("scaffold_planks", 11.0, -15.0, 0.5, (Box(4.0, 6.0, -4.0, 4.0, -6.0, -0.5),), ((2.0, 0.0), (8.5, -2.0))),
)
GIRDERS = (
    Segment("girder_beam", 13.0, 30.0, 0.0,
            (Box(3.0, 10.0, 1.5, 4.0, -6.0, -0.5), Box(3.0, 10.0, -4.0, -1.5, -6.0, -0.5)),
            ((1.5, 0.0), (6.5, 0.0), (11.5, 0.0))),
    Segment("girder_zigzag", 12.0, -15.0, 1.0, (Box(5.0, 7.0, -4.0, 4.0, -6.0, -0.5),), ((3.0, 2.5), (9.0, -2.5))),
)
HOOK = (  # the hook swings over open air, so every layout has a kill volume beyond the shared plane
    Segment("hook_swing", 12.0, 0.0, -1.5, (Box(3.0, 9.0, -4.0, 4.0, -6.0, -0.5),), ((1.5, 0.0), (10.5, 0.0))),
    Segment("hook_conveyor", 13.0, 15.0, -0.5, (Box(5.0, 8.0, -4.0, 0.0, -6.0, -0.5),), ((3.0, 2.0), (10.0, -2.0))),
)
LANDING = (
    Segment("sandpit", 11.0, 0.0, -1.0, (), ((3.0, -2.0), (6.0, 2.0))),
    Segment("dumpster", 12.0, 0.0, 0.0, (Box(4.0, 7.0, -4.0, -2.0, -2.0, -0.5),), ((2.0, 1.0), (8.0, 2.0))),
)
SECTION_NAMES = ("crane jib", "scaffolding", "girders", "hook ride", "sandpit/dumpster")
SKY = MapDef("sky", (JIB, SCAFFOLD, GIRDERS, HOOK, LANDING))

START_LOCAL = (1.0, 0.0, 0.1)   # in section 1, like Backyard's START
GATE_BACK = 1.0                 # finish gate this far before the end of section 5
PLANE_MARGIN, PLANE_DEPTH = 10.0, (-12.0, -8.0)

SkyLayout = namedtuple("SkyLayout", "layout start gate kill_plane kills cells")


def _kill_plane(placed):
    xs = [v for p in placed for v in (p.entry.x, p.exit.x)]
    ys = [v for p in placed for v in (p.entry.y, p.exit.y)]
    low = min(v for p in placed for v in (p.entry.z, p.exit.z))
    return Box(min(xs) - PLANE_MARGIN, max(xs) + PLANE_MARGIN, min(ys) - PLANE_MARGIN, max(ys) + PLANE_MARGIN,
               low + PLANE_DEPTH[0], low + PLANE_DEPTH[1])


def build(seed):
    layout = generate(SKY, seed)
    first, last = layout.placed[0], layout.placed[-1]
    return SkyLayout(layout, _to_world(first.entry, *START_LOCAL),
                       _to_world(last.entry, last.segment.length - GATE_BACK, 0.0, 0.0),
                       _kill_plane(layout.placed), [k for p in layout.placed for k in p.kills],
                       [c for p in layout.placed for c in p.cells])


def validate(sky):
    """Req 2-3 via check_layout, plus Req 5-6. Raises ValueError."""
    check_layout(sky.layout)
    if len(sky.layout.placed) != len(SECTION_NAMES):
        raise ValueError(f"sky has {len(sky.layout.placed)} sections, needs {len(SECTION_NAMES)}")
    for name, variants, p in zip(SECTION_NAMES, SKY.sections, sky.layout.placed):
        if p.segment not in variants:
            raise ValueError(f"section {name} holds {p.segment.name}, which is not one of its variants")
    if not sky.kills:
        raise ValueError("sky layout has no kill volume beyond the shared kill plane")
    if not 0 < len(sky.cells) <= budget():
        raise ValueError(f"{len(sky.cells)} build cells, needs 1-{budget()}")
    if sky.kill_plane.max_z >= min(p.entry.z for p in sky.layout.placed):
        raise ValueError("kill plane is not below the path")
