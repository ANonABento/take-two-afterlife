"""Shared map library: grid, roles, kill plane, credits and validation used by every map layout.

Units are metres, +Z is up, the path floor top is at Z = 0. Map scripts (Scripts/backyard_layout.py, ...) build
their props on this and keep only what is theirs: zones, dressing, start, gate.
"""
from maplib.credits import Kit, credits_missing, meshes_by_kit
from maplib.grid import REPO, corridor, grid_meters, in_corridor
from maplib.kill import WORLD_KILL_Z, kill_plane
from maplib.roles import ROLES, Box, Point, Prop, footprint, palette_index
from maplib.validate import validate

__all__ = ["REPO", "ROLES", "WORLD_KILL_Z", "Box", "Kit", "Point", "Prop", "corridor", "credits_missing",
           "footprint", "grid_meters", "in_corridor", "kill_plane", "meshes_by_kit", "palette_index", "validate"]
