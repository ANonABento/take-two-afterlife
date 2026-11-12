# blender -b --factory-startup --python-exit-code 1 --python build_heaven.py -- <outdir>
# Heaven cinema map meshes (TOON/GAME). Layout matches TTACinema.cpp: couch at origin facing +X,
# 4 seats 0.9 m apart, seat height 0.45 m, screen centre (12, 0, 3.5) m, 9.6 x 5.4 m. Cloud top at z=0.
import bpy, bmesh, math, sys, os, random
out = os.path.abspath(sys.argv[sys.argv.index("--") + 1])
os.makedirs(out, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
random.seed(7)

def mat(name, rgb):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = 0.8
    m.diffuse_color = (*rgb, 1); return m
M = {k: mat(k, v) for k, v in {
    "Cloud": (0.95, 0.95, 1.0), "CloudShade": (0.80, 0.84, 0.97), "Couch": (0.95, 0.62, 0.72),
    "Cushion": (1.0, 0.82, 0.86), "Gold": (0.95, 0.75, 0.30), "Screen": (0.06, 0.07, 0.10)}.items()}

def box(name, size, loc, m, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc); o = bpy.context.object
    o.name = name; o.scale = size; bpy.ops.object.transform_apply(scale=True)
    o.data.materials.append(M[m])
    if bevel:
        md = o.modifiers.new("b", "BEVEL"); md.width = bevel; md.segments = 3
    return o

def sphere(loc, r, sz, m):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=9, radius=r, location=loc)
    o = bpy.context.object; o.scale = (1, 1, sz); bpy.ops.object.transform_apply(scale=True)
    o.data.materials.append(M[m]); return o

def join(objs, name):
    for o in objs:
        o.select_set(True); bpy.context.view_layer.objects.active = o
        for md in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=md.name)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]; bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.object.shade_smooth(); o.data.use_auto_smooth = True; o.data.auto_smooth_angle = math.radians(40)
    bpy.ops.object.select_all(action="DESELECT"); return o

# Cloud: flat walkable disc (r 15 m, top z=0) + puffs around the rim and underneath.
bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=15, depth=0.8, location=(0, 0, -0.4))
disc = bpy.context.object; disc.data.materials.append(M["Cloud"])
parts = [disc]
for i in range(22):
    a = i / 22 * math.tau + random.uniform(-0.1, 0.1); r = random.uniform(1.6, 2.6)
    parts.append(sphere((15 * math.cos(a), 15 * math.sin(a), -0.6), r, 0.55, "Cloud"))
for i in range(14):
    a = random.uniform(0, math.tau); d = random.uniform(0, 11)
    parts.append(sphere((d * math.cos(a), d * math.sin(a), -1.4), random.uniform(2.5, 4), 0.45, "CloudShade"))
cloud = join(parts, "SM_Heaven_Cloud")

# Couch: 3.6 m wide (4 x 0.9), seat top 0.45 m, back on -X so sitters face +X.
W = 3.6
p = [box("base", (0.9, W + 0.3, 0.3), (0, 0, 0.15), "Couch", 0.06),
     box("back", (0.25, W + 0.3, 0.75), (-0.42, 0, 0.6), "Couch", 0.1),
     box("armL", (0.9, 0.3, 0.6), (0, -(W / 2 + 0.15), 0.4), "Couch", 0.12),
     box("armR", (0.9, 0.3, 0.6), (0, W / 2 + 0.15, 0.4), "Couch", 0.12)]
for s in range(4):
    y = (s - 1.5) * 0.9
    p.append(box(f"seat{s}", (0.68, 0.86, 0.15), (0.06, y, 0.375), "Cushion", 0.06))
    p.append(box(f"pillow{s}", (0.15, 0.7, 0.42), (-0.24, y, 0.68), "Cushion", 0.07))
couch = join(p, "SM_Heaven_Couch")

# Screen: gold frame + dark face on the -X side (towards the couch), centre (12,0,3.5).
p = [box("frame", (0.2, 9.9, 5.7), (12.05, 0, 3.5), "Gold", 0.05),
     box("face", (0.04, 9.6, 5.4), (11.94, 0, 3.5), "Screen"),
     box("legL", (0.3, 0.3, 0.8), (12.05, -3.5, 0.3), "Gold", 0.05),
     box("legR", (0.3, 0.3, 0.8), (12.05, 3.5, 0.3), "Gold", 0.05)]
screen = join(p, "SM_Heaven_Screen")

for o in (cloud, couch, screen):
    o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(); bpy.ops.object.mode_set(mode="OBJECT"); o.select_set(False)

# Pivots match TTACinema: cloud at actor origin, couch at (0,0,SeatHeight), screen at ScreenLocation.
from mathutils import Matrix, Vector
PIVOT = {cloud: Vector((0, 0, 0)), couch: Vector((0, 0, 0.45)), screen: Vector((12, 0, 3.5))}
for o, pv in PIVOT.items():
    o.data.transform(Matrix.Translation(-pv)); o.location = pv

tris = {o.name: sum(len(f.vertices) - 2 for f in o.data.polygons) for o in (cloud, couch, screen)}
print("TRIS", tris)
for o in (cloud, couch, screen):
    bpy.ops.object.select_all(action="DESELECT"); o.select_set(True); keep = o.location.copy(); o.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=f"{out}/{o.name}.fbx", use_selection=True, apply_unit_scale=True,
                             mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z")
    o.location = keep
bpy.ops.object.select_all(action="DESELECT")
bpy.ops.export_scene.gltf(filepath=f"{out}/heaven.glb", export_format="GLB", export_apply=True)

# Preview render (Cycles CPU, no OIDN on Debian build), toon-ish flat light.
sc = bpy.context.scene; sc.render.engine = "CYCLES"; sc.cycles.samples = 32; sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y = 800, 450
sc.view_settings.view_transform = "Standard"
w = bpy.data.worlds.new("w"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (1.0, 0.85, 0.65, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.9
bpy.ops.object.light_add(type="SUN", rotation=(math.radians(40), math.radians(20), math.radians(30))); bpy.context.object.data.energy = 3
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); sc.collection.objects.link(cam); sc.camera = cam
def shot(name, loc, target, lens=30):
    cam.location = loc; cam.data.lens = lens
    d = [t - l for t, l in zip(target, loc)]
    from mathutils import Vector
    cam.rotation_euler = Vector(d).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = f"{out}/{name}.png"; bpy.ops.render.render(write_still=True)
shot("heaven_overview", (-14, -16, 11), (3, 0, 0.5), 24)
shot("heaven_couch", (-3.5, -4.5, 2.2), (6, 0, 2.0), 22)
bpy.ops.wm.save_as_mainfile(filepath=f"{out}/heaven.blend", compress=True)
