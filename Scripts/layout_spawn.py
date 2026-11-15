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
MIN_SCALE, MAX_SCALE = 0.05, 10.0


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


def delete_previous(subsystem):
    """Destroy every TTA_Layout actor in the open level. Returns how many went."""
    gone = 0
    for actor in subsystem.get_all_level_actors():
        if is_stale(actor.get_class().get_name(), actor.get_editor_property("tags")):
            subsystem.destroy_actor(actor)
            gone += 1
    return gone


def tag(unreal, actor, *extra):
    actor.set_editor_property("tags", [unreal.Name(LAYOUT_TAG)] + [unreal.Name(t) for t in extra])


def spawn_fitted(unreal, subsystem, label, mesh, fit, target, yaw, extra_tags=()):
    """StaticMeshActor of `mesh` fitted to `fit` with its bottom-centre at `target` (cm), tagged TTA_Layout."""
    box = mesh.get_bounding_box()
    lo, hi = (box.min.x, box.min.y, box.min.z), (box.max.x, box.max.y, box.max.z)
    scale = fit_scale([h - l for h, l in zip(hi, lo)], fit)
    loc = placement(target, yaw, lo, hi, scale)
    actor = subsystem.spawn_actor_from_object(mesh, unreal.Vector(*loc), unreal.Rotator(0.0, 0.0, float(yaw)))
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_location(unreal.Vector(*loc), False, False)
    tag(unreal, actor, *extra_tags)
    return actor, scale


# Heaven target sizes (cm). Bounds-fitted, so a metres or centimetres import both land at this size.
HEAVEN_FIT = {
    "SM_Heaven_Cloud_A": ("width", 600.0),
    "SM_Heaven_Cloud_B": ("width", 600.0),
    "SM_Heaven_Cloud_C": ("width", 600.0),
    "SM_Heaven_Podium": ("footprint", (300.0, 300.0)),
    "SM_Heaven_FilmReelTrophy": ("height", 80.0),
    "SM_Heaven_Halo": ("width", 60.0),
    "SM_Heaven_VelvetRopeStanchions": ("height", 100.0),
}
HEAVEN_PODIUM_X = 780.0


def heaven_pieces(layout):
    """Every mesh dress_heaven.py spawns: dicts label, mesh, fit (cm), at (x, y, z cm), yaw, colour.

    The carpet's spot and size come from the layout's protected RedCarpet box, the dressing's from each
    entry's `half` (clouds) or HEAVEN_FIT. Trophy and Halo `at` are filled in from the podium's real height.
    """
    prot = {p["label"]: p for p in layout["protected"]}
    carpet = prot["RedCarpet"]
    out = [
        dict(label="SM_Heaven_Cloud_A", mesh="SM_Heaven_Cloud_A", at=(-600.0, -1400.0, 280.0)),
        dict(label="SM_Heaven_Cloud_B", mesh="SM_Heaven_Cloud_B", at=(1800.0, 1100.0, 420.0)),
        dict(label="SM_Heaven_Cloud_C", mesh="SM_Heaven_Cloud_C", at=(1600.0, -1600.0, 180.0)),
        dict(label="Podium", mesh="SM_Heaven_Podium", at=(HEAVEN_PODIUM_X, 0.0, 0.0), yaw=180.0, colour="Wood"),
        dict(label="Trophy", mesh="SM_Heaven_FilmReelTrophy", at=None, yaw=180.0, colour="Yellow"),
        dict(label="Halo", mesh="SM_Heaven_Halo", at=None, colour="Yellow"),
        dict(label="RedCarpet", mesh="SM_Heaven_RedCarpet", at=(carpet["x"], carpet["y"], 0.0), colour="Red",
             fit=("footprint", (carpet["half"][0] * 2, carpet["half"][1] * 2))),
    ]
    for p in out[:3]:
        p.update(yaw=0.0, colour="Sky" if p["mesh"].endswith("_B") else "Bone")
    for piece in layout["podium_dressing"] + layout["spectator_ring"]:
        fit = HEAVEN_FIT[piece["mesh"]]
        if fit[0] == "width":
            fit = ("width", 2.0 * max(piece["half"]))
        out.append(dict(label=piece["label"], mesh=piece["mesh"], fit=fit, at=(piece["x"], piece["y"], 0.0),
                        yaw=float(piece["yaw"]), colour=piece["colour"]))
    for p in out:
        p.setdefault("yaw", 0.0)
        p.setdefault("fit", HEAVEN_FIT.get(p["mesh"]))
    return out
