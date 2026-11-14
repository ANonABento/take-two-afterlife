# Task 6: rotate pair hats to spread on X, rescale all hats to the angel's head.
# Usage: blender -b --factory-startup --python-exit-code 1 --python fix_hats.py -- <in_dir> <out_dir>
# Head widths measured on final/angel.glb (head_hat socket at y=1.000 = top of head):
#   width at y 0.95 = 0.318 m, 0.97 = ~0.27 m, 0.98 = 0.224 m, 1.00 = 0.13 m.
# Each hat keeps its origin at base centre. SIT = base height offset from head_hat (m) for the engine.
import bpy, sys, os, math, json, mathutils
a = sys.argv[sys.argv.index("--")+1:]; IN, OUT = os.path.abspath(a[0]), os.path.abspath(a[1]); ONLY = a[2:]  # optional hat names (inputs must be un-fixed originals)
# name: (target width along X in m, rotate 90 about up?, sit offset from head_hat)
SPEC = {
 "crown":            (0.33, False, -0.05),
 "viking-helmet":    (0.34, False, -0.06),
 "propeller-beanie": (0.33, False, -0.05),
 "party-cone":       (0.21, False, -0.02),
 "traffic-cone":     (0.21, False, -0.02),
 "devil-horns":      (0.28, True,  -0.03),
 "bunny-ears":       (0.24, True,  -0.03),
 "halo":             (0.30, False, +0.06),
}
report = {}
for n, (W, rot, sit) in SPEC.items():
    if ONLY and n not in ONLY: continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(IN, n + ".glb"))
    ms = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    for o in bpy.context.scene.objects: o.select_set(o in ms)
    bpy.context.view_layer.objects.active = ms[0]
    if len(ms) > 1: bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.parent = None
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in list(bpy.context.scene.objects):
        if o != ob: bpy.data.objects.remove(o)
    if rot:
        ob.data.transform(mathutils.Matrix.Rotation(math.radians(90), 4, 'Z'))
    vs = [v.co for v in ob.data.vertices]
    mn = mathutils.Vector([min(v[i] for v in vs) for i in range(3)])
    mx = mathutils.Vector([max(v[i] for v in vs) for i in range(3)])
    s = W / (mx.x - mn.x)
    ob.data.transform(mathutils.Matrix.Translation((-(mn.x+mx.x)/2, -(mn.y+mx.y)/2, -mn.z)))
    ob.data.transform(mathutils.Matrix.Scale(s, 4))
    ob.location = (0, 0, 0); ob.name = n; ob.data.name = n
    ob.data.update(); vz = [v.co for v in ob.data.vertices]; d = mathutils.Vector([max(v[i] for v in vz) - min(v[i] for v in vz) for i in range(3)]); tris = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    report[n] = {"dims_xyz_m(blender,Z up)": [round(d.x,3), round(d.y,3), round(d.z,3)], "tris": tris,
                 "scale": round(s,3), "sit_offset_from_head_hat_m": sit}
    print("HAT", n, report[n])
    od = os.path.join(OUT, n); os.makedirs(od, exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
    bpy.ops.export_scene.gltf(filepath=os.path.join(od, n + ".glb"), use_selection=True, export_format='GLB')
    bpy.ops.export_scene.fbx(filepath=os.path.join(od, n + ".fbx"), use_selection=True, add_leaf_bones=False,
                             mesh_smooth_type='FACE', apply_scale_options='FBX_SCALE_UNITS')
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(od, n + ".blend"), compress=True)
fp = os.path.join(OUT, "hat_fit.json"); old = json.load(open(fp)) if os.path.exists(fp) else {}
old.update(report); json.dump(old, open(fp, "w"), indent=1)
