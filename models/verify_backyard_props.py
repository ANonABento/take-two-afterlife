"""Re-import each backyard FBX in a clean Blender scene; check dims (±5%), pivot z=0 (min z), tris < 3000.
Run: blender -b --factory-startup --python-exit-code 1 --python verify_backyard_props.py -- <Props dir>"""
import bpy, sys, os
from mathutils import Vector
d = sys.argv[sys.argv.index("--") + 1]
SPEC = {"Trampoline": (300, 300, 90), "KiddiePool": (150, 150, 30), "Shed": (250, 200, 230),
        "Grill": (None, None, 100), "PicnicTable": (180, 150, 75), "Gnome": (None, None, 40)}
bad = 0
for k, spec in SPEC.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(d, f"SM_Backyard_{k}.fbx"))
    o = [x for x in bpy.context.scene.objects if x.type == "MESH"][0]
    bpy.context.view_layer.update()
    cs = [o.matrix_world @ v.co for v in o.data.vertices]
    mn = Vector([min(c[i] for c in cs) for i in range(3)]); mx = Vector([max(c[i] for c in cs) for i in range(3)])
    dims = (mx - mn) * 100; tri = sum(len(p.vertices) - 2 for p in o.data.polygons)
    ok = all(s is None or abs(dims[i] - s) / s <= .05 for i, s in enumerate(spec)) and abs(mn.z) < 1e-3 and tri < 3000 \
         and abs(o.matrix_world.translation.length) < 1e-4 and abs((mx.x + mn.x) / 2) < .01 and abs((mx.y + mn.y) / 2) < .01
    bad += not ok
    print(f"VERIFY {k}: {dims.x:.1f}x{dims.y:.1f}x{dims.z:.1f} cm minZ={mn.z*100:.2f} cm pivot={tuple(round(t,4) for t in o.matrix_world.translation)} "
          f"scale={tuple(round(s,3) for s in o.scale)} tris={tri} {'PASS' if ok else 'FAIL'}")
print("ALL PASS" if not bad else f"{bad} FAIL"); sys.exit(1 if bad else 0)
