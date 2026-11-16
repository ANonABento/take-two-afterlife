"""What a placed prop is and how big it is."""
from collections import namedtuple

import toon_spec

# role: floor (walkable, blocks), marker (flat route sign on the floor, no collision), dressing (outside the
# corridor), shortcut (flat stepping stone of an alternate branch, inside the corridor, no collision), gate
# (finish arch, no collision), backdrop (seen, never touched: no collision, e.g. trench water). fit: ("box", (x, y, z)) scales each axis to that size;
# ("height", h) scales uniformly to that height. Position = bottom-centre of the fitted mesh.
ROLES = ("floor", "marker", "shortcut", "dressing", "gate", "backdrop", "scenery")   # scenery: a real mesh, seen and not felt
Prop = namedtuple("Prop", "kit mesh role x y z yaw palette fit")
Point = namedtuple("Point", "x y z")
Box = namedtuple("Box", "min_x max_x min_y max_y min_z max_z")


def palette_index(name):
    names = [n for n, _ in toon_spec.PALETTE]
    if name not in names:
        raise ValueError(f"{name!r} is not a palette colour")
    return names.index(name)


def footprint(p):
    """Rough XY half-size of a prop (m): box fits are exact; height fits assume a footprint up to its height."""
    kind, size = p.fit
    if kind == "box":
        hx, hy = size[0] / 2, size[1] / 2
        if p.yaw % 180 == 90:
            return hy, hx
        if p.yaw % 90:                      # turned box: its bounding circle
            r = (hx * hx + hy * hy) ** 0.5
            return r, r
        return hx, hy
    return size / 2, size / 2
