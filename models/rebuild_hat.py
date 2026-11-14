# Task 9: rebuild one hat from its raw textured Tripo GLB.
# blender -b --factory-startup --python-exit-code 1 --python rebuild_hat.py -- <raw.glb> <name> <out_dir>
# Raw -> task-6 fit (rotate/scale/origin at base) -> collapse-decimate copy <=3k tris -> Smart UV -> Cycles bake
# raw colour (DIFFUSE/COLOR, selected-to-active, 1024 px) -> Smooth by Angle 30 deg -> glb/fbx/blend.
import bpy, sys, os, math, mathutils, time
t0 = time.time()
a = sys.argv[sys.argv.index("--")+1:]; RAW, N, OUT = os.path.abspath(a[0]), a[1], os.path.abspath(a[2])
SPEC = {"crown": (0.33, False), "viking-helmet": (0.34, False), "propeller-beanie": (0.33, False),
        "party-cone": (0.21, False), "traffic-cone": (0.21, False), "devil-horns": (0.28, True),
        "bunny-ears": (0.24, True), "halo": (0.30, False)}
W, ROT = SPEC[N]; TRI_MAX = 2900; RES = 1024
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=RAW)
ms = [o for o in bpy.context.scene.objects if o.type == 'MESH']
for o in bpy.context.scene.objects: o.select_set(o in ms)
bpy.context.view_layer.objects.active = ms[0]
if len(ms) > 1: bpy.ops.object.join()
hi = bpy.context.view_layer.objects.active; hi.parent = None
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for o in list(bpy.context.scene.objects):
    if o != hi: bpy.data.objects.remove(o)
if ROT: hi.data.transform(mathutils.Matrix.Rotation(math.radians(90), 4, 'Z'))
vs = [v.co for v in hi.data.vertices]
mn = mathutils.Vector([min(v[i] for v in vs) for i in range(3)]); mx = mathutils.Vector([max(v[i] for v in vs) for i in range(3)])
s = W / (mx.x - mn.x)
hi.data.transform(mathutils.Matrix.Translation((-(mn.x+mx.x)/2, -(mn.y+mx.y)/2, -mn.z)))
hi.data.transform(mathutils.Matrix.Scale(s, 4)); hi.data.update(); hi.name = "RAW_hi"
hi_tris = sum(len(p.vertices)-2 for p in hi.data.polygons)
print("RAW", N, "tris", hi_tris, "t", round(time.time()-t0, 1))
# reduced copy
lo = hi.copy(); lo.data = hi.data.copy(); bpy.context.scene.collection.objects.link(lo)
bpy.ops.object.select_all(action='DESELECT'); lo.select_set(True); bpy.context.view_layer.objects.active = lo
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.remove_doubles(threshold=1e-5)
bpy.ops.object.mode_set(mode='OBJECT')
d = lo.modifiers.new("dec", 'DECIMATE'); d.ratio = TRI_MAX / hi_tris; d.use_collapse_triangulate = True
bpy.ops.object.modifier_apply(modifier="dec")
while lo.data.uv_layers: lo.data.uv_layers.remove(lo.data.uv_layers[0])
lo.data.uv_layers.new(name="UVMap")
lo.data.materials.clear()
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.01)
bpy.ops.object.mode_set(mode='OBJECT')
img = bpy.data.images.new(N + "_basecolor", RES, RES); img.generated_color = (1, 1, 1, 1)
mat = bpy.data.materials.new("M_SM_Hat_" + N.replace("-", "_").title().replace("_", "")); mat.use_nodes = True
nt = mat.node_tree; bsdf = nt.nodes["Principled BSDF"]; bsdf.inputs["Roughness"].default_value = 0.6
tex = nt.nodes.new("ShaderNodeTexImage"); tex.image = img; nt.nodes.active = tex
lo.data.materials.append(mat)
sc = bpy.context.scene; sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 1
bk = sc.render.bake; bk.use_selected_to_active = True; bk.cage_extrusion = 0.01; bk.max_ray_distance = 0.03; bk.margin = 8
hi.select_set(True); lo.select_set(True); bpy.context.view_layer.objects.active = lo
bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'})
nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
img.filepath_raw = os.path.join(OUT, N + "_basecolor.png"); img.file_format = 'PNG'; img.save(); img.pack()
# smooth by angle 30
bpy.ops.object.select_all(action='DESELECT'); lo.select_set(True); bpy.context.view_layer.objects.active = lo
bpy.ops.object.shade_smooth(); lo.data.use_auto_smooth = True; lo.data.auto_smooth_angle = math.radians(30)
for at in list(lo.data.attributes):
    if at.domain in ('POINT', 'CORNER') and at.data_type in ('FLOAT_COLOR', 'BYTE_COLOR'): lo.data.attributes.remove(at)
bpy.data.objects.remove(hi)
lo.name = N; lo.data.name = N; lo.location = (0, 0, 0)
tris = sum(len(p.vertices)-2 for p in lo.data.polygons)
dims = lo.dimensions
print("HAT", N, "tris", tris, "uv", [u.name for u in lo.data.uv_layers], "img", img.name, img.size[:], "dims", [round(x, 3) for x in dims], "t", round(time.time()-t0, 1))
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, N + ".glb"), use_selection=True, export_format='GLB')
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, N + ".fbx"), use_selection=True, add_leaf_bones=False,
                         mesh_smooth_type='FACE', apply_scale_options='FBX_SCALE_UNITS', path_mode='COPY', embed_textures=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, N + ".blend"), compress=True)
print("DONE", round(time.time()-t0, 1))
