# Measured hat fit for the gen 3 angel (faces +X, wings on -X, Z up).
# blender -b --factory-startup angel.blend --python-exit-code 1 --python fit_smart.py -- <hatdir> <outdir>
# Classes: band (crown, beanie, viking), perch (cones), pair (horns, ears on top), float (halo).
# Width cap: hat width <= 1.15 * max head width. No guessing turns: facing comes from measured features.
import bpy, sys, os, json, math, bmesh, numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
HD, OUT = [os.path.abspath(x) for x in sys.argv[sys.argv.index("--") + 1:]]
os.makedirs(os.path.join(OUT, "renders"), exist_ok=True)
HAT_HEAD_Z = 1.0
rig = bpy.data.objects["Rig"]; body = bpy.data.objects["Angel"]
dg = bpy.context.evaluated_depsgraph_get()
bm = bmesh.new(); bm.from_object(body, dg); bm.transform(body.matrix_world)
BV = BVHTree.FromBMesh(bm)
bw = np.array([v.co[:] for v in bm.verts]); bw = bw[bw[:, 0] > -0.25]   # drop wings
ZT = float(bw[:, 2].max())
def sl(z, t=0.006):
    s = bw[np.abs(bw[:, 2] - z) < t]; return s
def head_w(z): s = sl(z); return float(np.ptp(s[:, 1])) if len(s) > 3 else 0.0
zs = np.arange(ZT - 0.45, ZT, 0.005); ws = np.array([head_w(z) for z in zs])
HW = float(ws.max()); ZW = float(zs[ws.argmax()])          # widest head level (temple line)
top = sl(ZT - 0.04, 0.02); HCX, HCY = float(top[:, 0].mean()), float(top[:, 1].mean())
CAP = 1.15 * HW
print("HEAD", dict(top=ZT, width=HW, widest_z=ZW, cx=HCX, cy=HCY))
def inside(p):
    loc, n, i, d = BV.find_nearest(Vector(p))
    return loc is not None and (Vector(p) - loc).dot(n) < -1e-5
def n_inside(V): return int(sum(inside(p) for p in V))
def surf_z(x, y):  # head surface height under (x,y), raycast down
    loc, n, i, d = BV.ray_cast(Vector((x, y, ZT + 0.5)), Vector((0, 0, -1)))
    return (loc.z, n) if loc else (ZT, Vector((0, 0, 1)))
def drop(V, lo, hi):  # lowest z-shift in [lo,hi] with 0 verts inside (binary search on a sample)
    S = V[:: max(1, len(V) // 1500)]
    for _ in range(18):
        m = (lo + hi) / 2
        if n_inside(S + [0, 0, m]) == 0: hi = m
        else: lo = m
    while n_inside(V + [0, 0, hi]): hi += 0.001
    return hi
def conform(V, band_h, reach=0.04, gap=0.002):
    zmin = V[:, 2].min(); V = V.copy()
    for i, p in enumerate(V):
        loc, n, _, d = BV.find_nearest(Vector(p))
        if loc is None: continue
        sd = (Vector(p) - loc).dot(n)
        t = np.array(loc + n * gap)
        if sd < gap: V[i] = t                       # inside or touching: push to surface + gap
        elif p[2] - zmin < band_h and d < reach:    # lower band: pull rim onto head
            w = min(1.0, 1 - (p[2] - zmin - 0.01) / band_h); V[i] = p + w * (t - p)
    return V
def Rz(a): return np.array(Matrix.Rotation(math.radians(a), 3, 'Z'))
def Rx(a): return np.array(Matrix.Rotation(math.radians(a), 3, 'X'))
def Ry(a): return np.array(Matrix.Rotation(math.radians(a), 3, 'Y'))
CLS = {"crown": "band", "propeller-beanie": "band", "viking-helmet": "band", "party-cone": "perch",
       "traffic-cone": "perch", "devil-horns": "pair", "bunny-ears": "pair", "halo": "float"}
col = bpy.data.collections.new("Hats"); bpy.context.scene.collection.children.link(col)
fit, hats = {}, {}
for name, cls in CLS.items():
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(HD, name + ".glb"))
    new = set(bpy.data.objects) - before
    ob = [o for o in new if o.type == 'MESH'][0]; ob.name = "SM_Hat_" + name
    bpy.context.view_layer.update(); me = ob.data; me.transform(ob.matrix_world)
    for o in new:
        if o is not ob: bpy.data.objects.remove(o)
    for c in ob.users_collection: c.objects.unlink(ob)
    col.objects.link(ob); ob.parent = None; ob.matrix_world = Matrix.Identity(4)
    V = np.array([p.co[:] for p in me.vertices]); V -= [(V[:, 0].max() + V[:, 0].min()) / 2, (V[:, 1].max() + V[:, 1].min()) / 2, V[:, 2].min()]
    H = V[:, 2].max(); info = {"class": cls}
    if cls == "pair":
        # two parts sit side by side along the longer of X/Y -> that axis becomes Y (side to side)
        rot = 90 if np.ptp(V[:, 0]) > np.ptp(V[:, 1]) else 0
        V = V @ Rz(rot).T
        s = min(1.0, CAP * 0.75 / np.ptp(V[:, 1])) if np.ptp(V[:, 1]) > CAP * 0.75 else 1.0
        s = max(s, 0.6 * HW / np.ptp(V[:, 1]))     # spread so the pair spans ~60% of the head
        V = V * s; out = np.empty_like(V); parts = []
        for side in (1, -1):
            idx = V[:, 1] * side > 0; P = V[idx].copy()
            b = P[P[:, 2] < 0.15 * H * s]; bx, by = b[:, 0].mean(), b[:, 1].mean()
            P -= [bx, by, 0]
            P = P @ Rx(-8 * side).T                 # splay outward 12 deg
            y = HCY + side * 0.17 * HW; x = HCX
            z0, nrm = surf_z(x, y)
            P += [x, y, z0 - 0.02]
            dz = drop(P, -0.06, 0.08); P += [0, 0, dz]; P = conform(P, 0.15 * np.ptp(P[:, 2]), 0.02)
            out[idx] = P; parts.append([round(float(x), 4), round(float(y), 4), round(float(z0 - 0.02 + dz), 4)])
        V = out; info.update(rot_z_deg=rot, scale=round(s, 4), splay_deg=8, part_bases_world=parts)
    elif cls == "float":
        c = V - V.mean(0); _, _, vt = np.linalg.svd(c, full_matrices=False); nrm = vt[2] * np.sign(vt[2][2])
        R = np.array(Vector(nrm).rotation_difference(Vector((0, 0, 1))).to_matrix())
        V = (V - V.mean(0)) @ R.T; tilt = math.degrees(math.acos(min(1, abs(nrm[2]))))
        V += [HCX, HCY, ZT + 0.06 - V[:, 2].min()]
        info.update(rot_z_deg=0, scale=1.0, levelled_tilt_deg=round(tilt, 2))
    else:
        bot = V[V[:, 2] < 0.12 * H]; topv = V[V[:, 2] > 0.85 * H]
        if name == "crown":   # tallest point marks the front
            fx = topv[:, 0].mean(); fy = topv[:, 1].mean()
        else:                 # brim / nose guard: the low part sticks out at the front
            fx = bot[:, 0].mean(); fy = bot[:, 1].mean()
        rot = 0.0
        if cls == "band":
            ang = math.degrees(math.atan2(fy, fx))
            rot = -round(ang / 90) * 90 % 360          # snap the measured front to +X
            if name == "viking-helmet" and np.ptp(V[:, 1]) > np.ptp(V[:, 0]): rot = 0 if fx >= 0 else 180  # horns stay side-to-side
        V = V @ Rz(rot).T
        if cls == "band":
            seat = {"crown": ZT - 0.10, "propeller-beanie": ZT - 0.17, "viking-helmet": ZT - 0.18}[name]
            ring = V[V[:, 2] < 0.12 * H]
            s = head_w(seat) * 1.04 / np.ptp(ring[:, 1])
            s = min(s, CAP / max(np.ptp(V[:, 0]), np.ptp(V[:, 1])))
            V = V * s
            tilt = {"crown": 0, "propeller-beanie": 8, "viking-helmet": 6}[name]
            V = V @ Ry(tilt).T; V[:, 2] -= V[:, 2].min()
            V += [HCX, HCY, seat]; V = conform(V, 0.15 * np.ptp(V[:, 2]))
            # solid hat: geometry hidden inside the skull is removed (not visible), outside untouched
            info.update(rot_z_deg=rot, scale=round(float(s), 4), tilt_fwd_deg=tilt, seat_z=round(seat, 4))
        else:  # perch
            s = 0.70 * head_w(ZT - 0.03) / np.ptp(V[:, 1]); V = V * s
            V += [HCX + 0.02, HCY, ZT - 0.05]
            dz = drop(V, -0.05, 0.10); V += [0, 0, dz]; V = conform(V, 0.15 * np.ptp(V[:, 2]))
            info.update(rot_z_deg=rot, scale=round(float(s), 4))
    for i, p in enumerate(me.vertices): p.co = Vector(V[i])
    me.update()
    trimmed = 0
    if False:
        b2 = bmesh.new(); b2.from_mesh(me)
        dead = [v for v in b2.verts if inside(v.co)]; trimmed = len(dead)
        bmesh.ops.delete(b2, geom=dead, context='VERTS'); b2.to_mesh(me); b2.free(); me.update()
    W = np.array([p.co[:] for p in me.vertices])
    # checks
    ins = n_inside(W); wdt = float(max(np.ptp(W[:, 0]), np.ptp(W[:, 1])))
    chk = {"verts_inside_head": ins, "width_m": round(wdt, 3), "width_ok": wdt <= CAP + 1e-3}
    if cls == "pair":
        L = W[W[:, 1] > HCY]; Rr = W[W[:, 1] < HCY]
        chk["span_m"] = round(float(np.ptp(W[:, 1])), 3); chk["span_ok"] = chk["span_m"] <= HW
        chk["mirror_err_m"] = round(float(np.abs(np.array([L[:, 0].mean(), L[:, 1].mean() - HCY, L[:, 2].mean()]) -
                                               np.array([Rr[:, 0].mean(), HCY - Rr[:, 1].mean(), Rr[:, 2].mean()])).max()), 4)
    if cls in ("band", "perch"):
        rim = W[W[:, 2] < W[:, 2].min() + 0.01]
        g = np.array([BV.find_nearest(Vector(p))[3] for p in rim[:: max(1, len(rim) // 300)]]); g = g[g < 0.04]  # inner rim only (brim excluded)
        chk["rim_gap_ok"] = bool(np.percentile(g, 95) < 0.004)
        chk["rim_gap_max_mm"] = round(1000 * float(np.percentile(g, 95)), 1)
    org = Vector((HCX, HCY, float(W[:, 2].min())))
    me.transform(Matrix.Translation(-org))
    ob.parent = rig; ob.parent_type = 'BONE'; ob.parent_bone = "head_hat"
    ob.matrix_world = Matrix.Translation(org); bpy.context.view_layer.update()
    info.update(offset_from_head_hat_m=[round(org.x, 4), round(org.y, 4), round(org.z - HAT_HEAD_Z, 4)],
                bone_local_location=[round(x, 4) for x in ob.location], dims_m=[round(float(x), 3) for x in np.ptp(W, 0)],
                tris=sum(len(p.vertices) - 2 for p in me.polygons), trimmed_hidden_verts=trimmed, checks=chk)
    fit[name] = info; hats[name] = ob; print("FIT", name, json.dumps(info))
json.dump({"_note": "Gen 3 angel, faces +X. Hats are baked (turn, scale, tilt/splay applied); origin = base centre over the head. "
           "Parent to head_hat, place origin at offset_from_head_hat_m (world, rest pose, head_hat head z=1.0). "
           f"Head width {HW:.3f} m, hat width cap {CAP:.3f} m (head +15%).", **fit},
          open(os.path.join(OUT, "hat_fit.json"), "w"), indent=1)
sc = bpy.context.scene; sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'TEXTURE'
sc.display.shading.color_type = 'MATERIAL'
for m in bpy.data.materials:   # flat colour = mean of base texture, else base colour (no node deletion)
    if not m.use_nodes: continue
    col_ = None
    for n in m.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image is not None and n.image.has_data and n.image.size[0] > 0:
            px = np.empty(n.image.size[0] * n.image.size[1] * 4, np.float32); n.image.pixels.foreach_get(px)
            col_ = list(px.reshape(-1, 4)[:, :3].mean(0)) + [1]; break
    if col_ is None:
        b = m.node_tree.nodes.get("Principled BSDF")
        if b: col_ = list(b.inputs["Base Color"].default_value)
    if col_: m.diffuse_color = col_
sc.render.resolution_x = sc.render.resolution_y = 260; sc.render.film_transparent = False
w = bpy.data.worlds.new("W"); sc.world = w; w.color = (0.75, 0.75, 0.78)
cam = bpy.data.objects.new("PrevCam", bpy.data.cameras.new("PrevCam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 0.85
tgt = Vector((HCX, HCY, ZT - 0.05))
for view, d in (("front", (1, 0, 0.08)), ("side", (0, -1, 0.05)), ("top", (0.001, 0, 1))):
    cam.location = tgt + Vector(d).normalized() * 4
    cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
    for n in hats:
        for m, o in hats.items(): o.hide_render = (m != n)
        sc.render.filepath = os.path.join(OUT, "renders", f"{view}_{n}.png"); bpy.ops.render.render(write_still=True)
print("DONE")
