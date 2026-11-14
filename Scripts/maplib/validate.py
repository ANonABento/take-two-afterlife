"""Map rules every layout must pass. Raises ValueError naming the first broken rule."""
from maplib.grid import in_corridor
from maplib.kill import WORLD_KILL_Z
from maplib.roles import ROLES, footprint, palette_index


def validate(props, kits, length, width, start, gate, kill, ditch, outer_width, floor_thickness):
    """Check a straight path along +X: x in [0, length], |y| <= width / 2, open ditch and outer strip each side."""
    half = width / 2
    for p in props:
        if p.kit not in kits:
            raise ValueError(f"{p.mesh}: unknown kit {p.kit}")
        if p.role not in ROLES:
            raise ValueError(f"{p.mesh}: unknown role {p.role}")
        palette_index(p.palette)
        hx, hy = footprint(p)
        if p.role == "dressing":
            # nothing inside the corridor: traps own it, the route stays clear
            if p.x + hx > 0 and p.x - hx < length and abs(p.y) - hy < half + ditch:
                raise ValueError(f"dressing {p.mesh} at ({p.x}, {p.y}) intrudes on the corridor or ditch")
        elif p.role == "marker":
            if not in_corridor(p.x, p.y, length, half) or p.fit[1][2] > 0.05:
                raise ValueError(f"marker {p.mesh} at ({p.x}, {p.y}) must be flat and inside the corridor")
    floors = [p for p in props if p.role == "floor" and in_corridor(p.x, p.y, length, half)]
    covered = sorted((p.x - footprint(p)[0], p.x + footprint(p)[0]) for p in floors)
    reach = 0.0
    for lo, hi in covered:
        if lo > reach + 1e-6:
            raise ValueError(f"corridor floor has a gap at x = {reach}")
        reach = max(reach, hi)
    if reach < length:
        raise ValueError(f"corridor floor stops at x = {reach}")
    if not (in_corridor(start.x, start.y, length, half) and in_corridor(gate.x, gate.y, length, half)):
        raise ValueError("start and gate must be inside the corridor")
    if gate.x - start.x < 0.9 * length:
        raise ValueError("route from start to gate is shorter than 90% of the corridor")
    k = kill
    if not (k.min_x < 0 and k.max_x > length and k.min_y < -half - ditch - outer_width
            and k.max_y > half + ditch + outer_width):
        raise ValueError("kill plane does not cover the map footprint")
    if not (WORLD_KILL_Z < k.min_z < k.max_z < -floor_thickness - 3.0):
        raise ValueError("kill plane must sit well below the floor and above the engine KillZ")
