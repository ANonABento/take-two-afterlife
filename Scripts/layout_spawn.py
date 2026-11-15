"""Shared spawn helper for the layout scripts (build_backyard_map.py, dress_heaven.py). No unreal import.

All sizes here are centimetres (UE units). Meshes are fitted to a target size from their bounding box,
once, and the actor itself is placed so the mesh's bottom-centre lands on the target spot. Never move a
StaticMeshActor's static_mesh_component: it is the root, so its relative location IS the actor location.
"""
import math

LAYOUT_TAG = "TTA_Layout"     # every actor a layout script spawns; a rerun deletes these first
# Never deleted by a layout rerun, even if someone tagged them.
PROTECTED_CLASSES = ("PlayerStart", "TTAFinishGate", "TTAKillPlane", "NavMeshBoundsVolume", "WorldSettings",
                     "RecastNavMesh")
# Guard against unit mistakes (a metres mesh read as centimetres is 100x out). 0.02 still lets a 1 m tall
# kit piece be pressed into a 5 cm route marker.
MIN_SCALE, MAX_SCALE = 0.02, 10.0


def fit_scale(size, fit):
    """(sx, sy, sz) that brings a mesh of bounds `size` (cm) to `fit` (cm).

    fit: ("box", (x, y, z)) exact box; ("footprint", (x, y)) x/y exact, z = mean of the two;
    ("height", h) / ("width", w) uniform, from Z / the larger of X and Y.
    Raises ValueError when the scale leaves [MIN_SCALE, MAX_SCALE]: the import is in the wrong units.
    """
    sx, sy, sz = (max(float(v), 1e-6) for v in size)
    kind, target = fit
    if kind == "box":
        scale = (target[0] / sx, target[1] / sy, target[2] / sz)
    elif kind == "footprint":
        a, b = target[0] / sx, target[1] / sy
        scale = (a, b, (a + b) / 2)
    elif kind == "height":
        scale = (target / sz,) * 3
    elif kind == "width":
        scale = (target / max(sx, sy),) * 3
    else:
        raise ValueError(f"unknown fit {kind!r}")
    if not all(MIN_SCALE <= s <= MAX_SCALE for s in scale):
        raise ValueError(f"fit {fit} on bounds {size} gives scale {scale}: mesh imported in the wrong units?")
    return scale


def placement(target, yaw, box_min, box_max, scale):
    """Actor location (cm) so the scaled, yawed mesh's bounding-box bottom-centre sits on `target`."""
    cx = (box_min[0] + box_max[0]) / 2 * scale[0]
    cy = (box_min[1] + box_max[1]) / 2 * scale[1]
    cz = box_min[2] * scale[2]
    r = math.radians(yaw)
    ox, oy = cx * math.cos(r) - cy * math.sin(r), cx * math.sin(r) + cy * math.cos(r)
    return (target[0] - ox, target[1] - oy, target[2] - cz)


def is_stale(class_name, tags):
    """True for an actor an earlier layout run spawned (tagged TTA_Layout) that is safe to delete."""
    return LAYOUT_TAG in [str(t) for t in tags] and class_name not in PROTECTED_CLASSES


def delete_previous(subsystem, legacy=None):
    """Destroy every TTA_Layout actor in the open level. Returns how many went.

    legacy(class name, label) -> bool also removes untagged leftovers of runs from before the tag existed
    (those piled up as duplicates: two podiums, three suns). Protected classes never go.
    """
    gone = 0
    for actor in subsystem.get_all_level_actors():
        cls = actor.get_class().get_name()
        stale = is_stale(cls, actor.get_editor_property("tags"))
        if not stale and legacy is not None and cls not in PROTECTED_CLASSES:
            stale = bool(legacy(cls, str(actor.get_actor_label())))
        if stale:
            subsystem.destroy_actor(actor)
            gone += 1
    return gone


def tag(unreal, actor, *extra):
    actor.set_editor_property("tags", [unreal.Name(LAYOUT_TAG)] + [unreal.Name(t) for t in extra])


def mesh_bounds(mesh):
    box = mesh.get_bounding_box()
    return (box.min.x, box.min.y, box.min.z), (box.max.x, box.max.y, box.max.z)


def spawn_fitted(unreal, subsystem, label, mesh, fit, target, yaw, extra_tags=(), collide=True):
    """StaticMeshActor of `mesh` fitted to `fit` with its bottom-centre at `target` (cm), tagged TTA_Layout.

    `fit` is in mesh axes: a piece yawed 90 that must span the world Y axis is fitted along its own X.
    """
    lo, hi = mesh_bounds(mesh)
    scale = fit_scale([h - l for h, l in zip(hi, lo)], fit)
    loc = placement(target, yaw, lo, hi, scale)
    actor = subsystem.spawn_actor_from_object(mesh, unreal.Vector(*loc), unreal.Rotator(0.0, 0.0, float(yaw)))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_location(unreal.Vector(*loc), False, False)
    if not collide:
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    tag(unreal, actor, *extra_tags)
    return actor, scale


def rotate(x, y, yaw):
    r = math.radians(yaw)
    return x * math.cos(r) - y * math.sin(r), x * math.sin(r) + y * math.cos(r)


# ---- Heaven (cm, ATTACinema space: couch at the origin facing +X, screen at x = 1200) ----------------------
# Sizes are bounds-fitted, so a metres or centimetres import both land at this size.
HEAVEN_FIT = {
    "SM_Heaven_Cloud_A": ("width", 600.0),
    "SM_Heaven_Cloud_B": ("width", 600.0),
    "SM_Heaven_Cloud_C": ("width", 600.0),
    "SM_Heaven_Podium": ("width", 450.0),
    "SM_Heaven_FilmReelTrophy": ("height", 80.0),
    "SM_Heaven_Halo": ("width", 90.0),
    "SM_Heaven_VelvetRopeStanchions": ("height", 100.0),
    "SM_Heaven_PearlyGate": ("height", 700.0),
}
# SM_Heaven_Podium as modelled (models/SM_Heaven_Podium.glb): three blocks along X, number plates on -Y.
PODIUM_SIZE = (450.0, 152.0, 100.0)
PODIUM_STEPS = {"1st": (0.0, 100.0), "2nd": (-150.0, 70.0), "3rd": (150.0, 40.0)}   # tag -> (mesh x, top z)
STATUE_HALF_HEIGHT = 90.0   # runner capsule half height + 2: a statue is spawned by its centre
STATUE_HEIGHT = 180.0
GATE_TRIGGER_HALF = (50.0, 250.0, 250.0)   # ATTAFinishGate trigger, gate space: thin along the walk direction


def heaven_protected(layout):
    return {p["label"]: p for p in layout["protected"]}


def podium_spots(layout):
    """{tag: (x, y, z, yaw)}: where each statue's centre goes, facing the way the podium's plates face."""
    podium = heaven_protected(layout)["Podium"]
    facing = podium["yaw"] - 90.0   # plates are on the mesh's -Y side
    out = {}
    for name, (mx, top) in PODIUM_STEPS.items():
        dx, dy = rotate(mx, 0.0, podium["yaw"])
        out[name] = (podium["x"] + dx, podium["y"] + dy, top + STATUE_HALF_HEIGHT, facing % 360.0)
    return out


def heaven_pieces(layout):
    """Every mesh dress_heaven.py spawns: dicts label, mesh, fit (cm, mesh axes), at (x, y, z cm), yaw, colour,
    collide. Podium, carpet and gate spots come from the layout's protected boxes."""
    prot = heaven_protected(layout)
    carpet, podium = prot["RedCarpet"], prot["Podium"]
    first = podium_spots(layout)["1st"]
    out = [
        dict(label="SM_Heaven_Cloud_A", mesh="SM_Heaven_Cloud_A", at=(-600.0, -1700.0, 280.0), colour="Bone"),
        dict(label="SM_Heaven_Cloud_B", mesh="SM_Heaven_Cloud_B", at=(2100.0, 1300.0, 420.0), colour="Sky"),
        dict(label="SM_Heaven_Cloud_C", mesh="SM_Heaven_Cloud_C", at=(2000.0, -1700.0, 180.0), colour="Bone"),
        dict(label="Podium", mesh="SM_Heaven_Podium", at=(podium["x"], podium["y"], 0.0), yaw=podium["yaw"],
             colour="Wood"),
        # The halo hangs over the winner's head; the trophy stands on the floor in front of the plates.
        dict(label="Halo", mesh="SM_Heaven_Halo", at=(first[0], first[1], PODIUM_STEPS["1st"][1] + STATUE_HEIGHT + 45.0),
             colour="Yellow", collide=False),
        dict(label="Trophy", mesh="SM_Heaven_FilmReelTrophy", colour="Yellow", collide=False,
             at=tuple(a + b for a, b in zip((podium["x"], podium["y"], 0.0),
                                            rotate(0.0, -(PODIUM_SIZE[1] / 2 + 45.0), podium["yaw"]) + (0.0,))),
             yaw=podium["yaw"]),
        # The carpet mesh is long along its own Y.
        dict(label="RedCarpet", mesh="SM_Heaven_RedCarpet", at=(carpet["x"], carpet["y"], 0.0), colour="Red",
             fit=("footprint", (carpet["half"][0] * 2, carpet["half"][1] * 2)), collide=False),
    ]
    for piece in layout["podium_dressing"] + layout["spectator_ring"]:
        fit = HEAVEN_FIT[piece["mesh"]]
        if fit[0] == "width":
            fit = ("width", 2.0 * max(piece["half"]))
        out.append(dict(label=piece["label"], mesh=piece["mesh"], fit=fit, at=(piece["x"], piece["y"], 0.0),
                        yaw=float(piece["yaw"]), colour=piece["colour"]))
    for p in out:
        p.setdefault("yaw", 0.0)
        p.setdefault("collide", True)
        p.setdefault("fit", HEAVEN_FIT.get(p["mesh"]))
    return out


def heaven_labels(layout):
    """Labels of everything dress_heaven.py spawns (meshes and podium TargetPoints): the legacy purge list."""
    return {p["label"] for p in heaven_pieces(layout)} | {f"Podium_{t}" for t in PODIUM_STEPS}
