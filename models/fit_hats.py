# Compute scale/rotation/offset so each hat rests on the gen 3 angel head with no vertex inside it.
# python fit_hats.py <angel.glb> <hat dir with <name>.glb> <out hat_fit.json>
# All coords Blender (Z up). glTF (x,y,z) -> Blender (x,-z,y).
import sys, json, numpy as np, trimesh
ANG, HD, OUT = sys.argv[1:4]
HAT_HEAD_Z = 1.0          # head_hat bone head (rest), world
CLEAR = 0.003             # gap above the touch point
def bl(v): v = np.asarray(v); return np.c_[v[:, 0], -v[:, 2], v[:, 1]]
ang = trimesh.load(ANG).to_geometry(); ang.merge_vertices(merge_tex=True, merge_norm=True)
assert ang.is_watertight
av = bl(ang.vertices); body = trimesh.Trimesh(av, ang.faces, process=False)
def head_width_y(z):  # side-to-side head width (Blender Y) at height z
    s = av[np.abs(av[:, 2] - z) < 0.01]; return np.ptp(s[:, 1])
# name: (rotate about Z deg, mode, target base width height or None, extra)
SPEC = {
 "crown":            (0,  "seat", 0.90),
 "propeller-beanie": (90, "seat", 0.86),
 "viking-helmet":    (0,  "seat", 0.85),
 "party-cone":       (0,  "seat", 0.97),
 "traffic-cone":     (0,  "seat", 0.97),
 "devil-horns":      (90, "seat", 0.93),
 "bunny-ears":       (90, "seat", 0.95),
 "halo":             (0,  "float", None),
}
def top_of_head(xy):
    """first downward hit z on the body for each (x,y) column, nan if miss"""
    o = np.c_[xy, np.full(len(xy), 2.0)]; d = np.tile([0, 0, -1.0], (len(xy), 1))
    loc, ri, _ = body.ray.intersects_location(o, d, multiple_hits=False)
    z = np.full(len(xy), np.nan); z[ri] = loc[:, 2]; return z
FIXED = {"devil-horns": 1.2, "bunny-ears": 1.15}  # tip footprint is tiny; size by look
fit = {}
for name, (rot, mode, zt) in SPEC.items():
    g = trimesh.load(f"{HD}/{name}.glb", force="mesh")
    v = bl(g.vertices); tris = len(g.faces)
    c, s_ = np.cos(np.radians(rot)), np.sin(np.radians(rot))
    v = v @ np.array([[c, s_, 0], [-s_, c, 0], [0, 0, 1]])
    v[:, :2] -= (v[:, :2].max(0) + v[:, :2].min(0)) / 2; v[:, 2] -= v[:, 2].min()
    if mode == "float":
        scale = 1.0
    else:
        base = v[v[:, 2] < 0.15 * v[:, 2].max()]
        scale = head_width_y(zt) * (0.80 if name in ("party-cone", "traffic-cone") else 1.0) / np.ptp(base[:, 1])
        scale = float(np.clip(scale, 0.6, 1.8))
        if name in FIXED: scale = FIXED[name]
    vs = v * scale
    tz = top_of_head(vs[:, :2])
    hit = ~np.isnan(tz)
    if mode == "float":
        z0 = float(np.nanmax(top_of_head(np.array([[0.0, 0.0]]))) + 0.06)
    else:
        z0 = float(np.max(tz[hit] - vs[hit, 2])) + CLEAR
    world = vs + [0, 0, z0]
    inside = int(body.contains(world).sum())
    fit[name] = {"tris": tris, "rot_z_deg": rot, "scale": round(scale, 4),
                 "offset_from_head_hat_m": [0.0, 0.0, round(z0 - HAT_HEAD_Z, 4)],
                 "dims_m(blender,Z up)": [round(float(x), 3) for x in np.ptp(world, 0)],
                 "verts_inside_head": inside}
    print(name, fit[name])
json.dump({"_note": "Gen 3 angel (59,972 tris). Hat parented to bone head_hat (rest head at world z=1.0). "
           "Apply: rotate rot_z_deg about Z, uniform scale, then translate offset (world axes, rest pose; hat origin = base centre).",
           **fit}, open(OUT, "w"), indent=1)
