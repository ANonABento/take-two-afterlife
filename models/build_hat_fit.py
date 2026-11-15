# Parent the 8 hats to head_hat on the gen 3 angel using hat_fit.json; save .blend, .glb, per-hat world verts, preview.
# blender -b --factory-startup <angel.blend> --python-exit-code 1 --python build_hat_fit.py -- <hat dir> <hat_fit.json> <outdir>
import bpy, sys, os, json, math, mathutils, numpy as np
a = sys.argv[sys.argv.index("--") + 1:]; HD, FIT, OUT = [os.path.abspath(x) for x in a]
os.makedirs(OUT, exist_ok=True); os.makedirs(os.path.join(OUT, "renders"), exist_ok=True)
fit = {k: v for k, v in json.load(open(FIT)).items() if not k.startswith("_")}
rig = bpy.data.objects["Rig"]; body = bpy.data.objects["Angel"]
bone = rig.data.bones["head_hat"]
col = bpy.data.collections.new("Hats"); bpy.context.scene.collection.children.link(col)
hats = {}
for name, f in fit.items():
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(HD, name + ".glb"))
    new = [o for o in set(bpy.data.objects) - before if o.type == 'MESH']
    ob = new[0]; ob.name = "SM_Hat_" + name
    for o in set(bpy.data.objects) - before:
        for c in o.users_collection: c.objects.unlink(o)
        col.objects.link(o)
    bpy.context.view_layer.update()
    # bake glTF import rotation into mesh, put origin at base centre (same as fit_hats.py)
    me = ob.data; me.transform(ob.matrix_world); ob.matrix_world = mathutils.Matrix.Identity(4); me.update()
    v = np.array([p.co for p in me.vertices])
    ctr = mathutils.Vector(((v[:, 0].max() + v[:, 0].min()) / 2, (v[:, 1].max() + v[:, 1].min()) / 2, v[:, 2].min()))
    me.transform(mathutils.Matrix.Translation(-ctr)); me.update()
    for o in set(bpy.data.objects) - before:
        if o is not ob: bpy.data.objects.remove(o)
    world = (mathutils.Matrix.Translation(mathutils.Vector((0, 0, 1.0)) + mathutils.Vector(f["offset_from_head_hat_m"]))
             @ mathutils.Matrix.Rotation(math.radians(f["rot_z_deg"]), 4, 'Z')
             @ mathutils.Matrix.Scale(f["scale"], 4))
    ob.parent = rig; ob.parent_type = 'BONE'; ob.parent_bone = "head_hat"
    ob.matrix_world = world
    hats[name] = ob
bpy.context.view_layer.update()
dump = {"body": np.array([body.matrix_world @ v.co for v in body.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh().vertices])}
dump["body_faces"] = np.array([list(p.vertices) for p in body.data.polygons if len(p.vertices) == 3])
for n, ob in hats.items():
    dump[n] = np.array([ob.matrix_world @ v.co for v in ob.data.vertices])
    print("HAT", n, "parent", ob.parent.name, ob.parent_bone, "loc", tuple(round(x, 4) for x in ob.location),
          "scale", tuple(round(x, 4) for x in ob.scale), "dims", tuple(round(x, 3) for x in ob.dimensions))
np.savez_compressed(os.path.join(OUT, "hat_world_verts.npz"), **dump)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "angel_hats.blend"), compress=True)
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "angel_hats.glb"), export_format='GLB')
# previews: Workbench, one hat at a time, front and side
sc = bpy.context.scene; sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.color_type = 'TEXTURE'; sc.display.shading.light = 'STUDIO'
sc.render.resolution_x = sc.render.resolution_y = 300; sc.render.film_transparent = False
for o in bpy.data.objects:
    if o.type == 'MESH' and o.name not in hats and o is not body and o.name.startswith("SM_Hat_") is False and o.parent_bone in ("", None) and o is not body:
        pass
cam = bpy.data.objects.new("PrevCam", bpy.data.cameras.new("PrevCam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 0.95
tgt = mathutils.Vector((0, 0, 0.92))
for view, d in (("front", (-1, 0, 0.15)), ("side", (0, -1, 0.15))):
    cam.location = tgt + mathutils.Vector(d).normalized() * 4
    cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
    for n in hats:
        for m, o in hats.items(): o.hide_render = (m != n)
        sc.render.filepath = os.path.join(OUT, "renders", f"{view}_{n}.png"); bpy.ops.render.render(write_still=True)
