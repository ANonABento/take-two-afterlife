"""Shared kill plane: a box under the whole map; fallers hit it and respawn at the start."""
from maplib.roles import Box

WORLD_KILL_Z = -500.0   # engine KillZ, far below so ragdolls are respawned, not destroyed


def kill_plane(length, margin=10.0, half_y=20.0, min_z=-12.0, max_z=-8.0):
    """Kill box from x = -margin to length + margin, |y| <= half_y, between min_z and max_z."""
    return Box(-margin, length + margin, -half_y, half_y, min_z, max_z)
