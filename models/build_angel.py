# Task 12: Angel character. blender -b --factory-startup --python-exit-code 1 --python build_angel.py -- <outdir>
import bpy, bmesh, sys, os, math
from mathutils import Vector
OUT = os.path.abspath(sys.argv[sys.argv.index("--") + 1])
os.makedirs(OUT, exist_ok=True)
PAL = ["1E1B2E","F4F1E8","E5484D","F2913D","F5D547","6CC24A","2BB3A3","5BA8F5","3F5BD9","8E5CD9","F27BB0","9A6A43"]
IDX = {n: i for i, n in enumerate(["Ink","Bone","Red","Orange","Yellow","Grass","Teal","Sky","Blue","Purple","Pink","Wood"])}
def srgb(h):
    c = [int(h[i:i+2], 16) / 255 for i in (0, 2, 4)]
    return [x/12.92 if x <= 0.04045 else ((x+0.055)/1.055)**2.4 for x in c] + [1]

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene

# palette texture 12x1 (same as Scripts/toon_spec.py)
pimg = bpy.data.images.new("T_TTA_Palette", 12, 1)
px = []
for h in PAL: px += srgb(h)
pimg.pixels = px; pimg.filepath_raw = os.path.join(OUT, "T_TTA_Palette.png"); pimg.file_format = 'PNG'; pimg.save()
fimg = bpy.data.images.load(os.path.join(OUT, "T_Angel_Face_Default.png"))

def mat(name, img, col):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    t = m.node_tree.nodes.new("ShaderNodeTexImage"); t.image = img; t.interpolation = 'Closest'
    m.node_tree.links.new(t.outputs[0], b.inputs["Base Color"])
    b.inputs["Roughness"].default_value = 1.0
    m.diffuse_color = srgb(col)
    return m
M_PAL = mat("MI_TTA_Toon_Angel", pimg, PAL[1])
M_FACE = mat("MI_TTA_Toon_AngelFace", fimg, PAL[1])

def part(kind, loc, scale, colour, bone, seg=16, ring=8, rot=(0,0,0)):
    if kind == "sphere":
        bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=ring, location=loc, rotation=rot)
    elif kind == "cyl":
        bpy.ops.mesh.primitive_cylinder_add(vertices=seg, location=loc, rotation=rot)
    elif kind == "torus":
        bpy.ops.mesh.primitive_torus_add(major_segments=seg, minor_segments=ring, location=loc, rotation=rot,
                                         major_radius=1, minor_radius=0.12)
    o = bpy.context.object; o.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    o.data.materials.append(M_PAL); o.data.materials.append(M_FACE)
    uv = o.data.uv_layers.new(name="UVMap")
    u = (IDX[colour] + 0.5) / 12
    for l in uv.data: l.uv = (u, 0.5)
    o.vertex_groups.new(name=bone).add(range(len(o.data.vertices)), 1.0, 'REPLACE')
    return o

body = []
# head: big, top at 1.20 m; face region gets face material + planar UV (front = -Y)
head = part("sphere", (0, 0, 0.93), (0.27, 0.25, 0.27), "Bone", "head", 24, 14)
uvl = head.data.uv_layers[0]
for p in head.data.polygons:
    c = p.center
    if p.normal.y < -0.35:
        p.material_index = 1
        for li in p.loop_indices:
            v = head.data.vertices[head.data.loops[li].vertex_index].co
            uvl.data[li].uv = ((v.x + 0.27) / 0.54, (v.z - 0.66) / 0.54)
body.append(head)
body.append(part("sphere", (0, 0, 0.44), (0.17, 0.14, 0.12), "Bone", "pelvis"))   # hips
body.append(part("sphere", (0, 0, 0.58), (0.16, 0.13, 0.14), "Bone", "spine"))    # chest/robe
for s, sx in (("l", 1), ("r", -1)):
    body.append(part("cyl", (sx*0.25, 0, 0.58), (0.045, 0.045, 0.08), "Bone", f"upperarm_{s}", 10, rot=(0, sx*math.radians(40), 0)))
    body.append(part("cyl", (sx*0.33, 0, 0.47), (0.04, 0.04, 0.07), "Bone", f"lowerarm_{s}", 10, rot=(0, sx*math.radians(25), 0)))
    body.append(part("sphere", (sx*0.37, 0, 0.38), (0.055, 0.055, 0.055), "Bone", f"hand_{s}", 10, 6))
    body.append(part("cyl", (sx*0.08, 0, 0.30), (0.055, 0.055, 0.08), "Bone", f"thigh_{s}", 10))
    body.append(part("cyl", (sx*0.08, 0, 0.16), (0.05, 0.05, 0.07), "Bone", f"calf_{s}", 10))
    body.append(part("sphere", (sx*0.08, -0.03, 0.045), (0.065, 0.09, 0.045), "Bone", f"foot_{s}", 10, 6))
bpy.ops.object.select_all(action='DESELECT')
for o in body: o.select_set(True)
bpy.context.view_layer.objects.active = head
bpy.ops.object.join(); B = bpy.context.object; B.name = B.data.name = "SK_Angel"

# armature: 15 bones, Z up, character faces -Y
bpy.ops.object.armature_add(location=(0, 0, 0)); A = bpy.context.object; A.name = "Armature"; A.data.name = "Armature"
bpy.ops.object.mode_set(mode='EDIT'); eb = A.data.edit_bones; eb.remove(eb[0])
def bone(n, h, t, par=None):
    b = eb.new(n); b.head = h; b.tail = t
    if par: b.parent = eb[par]; b.use_connect = False
bone("pelvis", (0,0,0.38), (0,0,0.50)); bone("spine", (0,0,0.50), (0,0,0.68), "pelvis"); bone("head", (0,0,0.68), (0,0,1.20), "spine")
for s, sx in (("l", 1), ("r", -1)):
    bone(f"upperarm_{s}", (sx*0.20,0,0.64), (sx*0.30,0,0.52), "spine")
    bone(f"lowerarm_{s}", (sx*0.30,0,0.52), (sx*0.36,0,0.42), f"upperarm_{s}")
    bone(f"hand_{s}", (sx*0.36,0,0.42), (sx*0.39,0,0.33), f"lowerarm_{s}")
    bone(f"thigh_{s}", (sx*0.08,0,0.38), (sx*0.08,0,0.23), "pelvis")
    bone(f"calf_{s}", (sx*0.08,0,0.23), (sx*0.08,0,0.08), f"thigh_{s}")
    bone(f"foot_{s}", (sx*0.08,0,0.08), (sx*0.08,-0.11,0.04), f"calf_{s}")
bpy.ops.object.mode_set(mode='OBJECT')
B.parent = A; mod = B.modifiers.new("Armature", 'ARMATURE'); mod.object = A

# hat socket (empty, parented to head bone) -> recreate as skeletal socket "HatSocket" on head in UE
def socket(name, loc, bn):
    e = bpy.data.objects.new(name, None); sc.collection.objects.link(e); e.empty_display_size = 0.05
    e.parent = A; e.parent_type = 'BONE'; e.parent_bone = bn
    e.matrix_world = e.matrix_world.Translation(Vector(loc)) if False else e.matrix_world
    e.matrix_parent_inverse.identity(); bpy.context.view_layer.update()
    e.matrix_world = __import__("mathutils").Matrix.Translation(loc)
    return e
socket("SOCKET_Hat", (0, 0, 1.17), "head"); socket("SOCKET_Halo", (0, 0, 1.30), "head"); socket("SOCKET_Wings", (0, 0.12, 0.60), "spine")

# detachable accessories (static, origin = their socket point)
def accessory(objs, name, origin):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.vertex_groups.clear(); o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]; bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    sc.cursor.location = origin; bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); sc.cursor.location = (0,0,0)
    o.data.materials.pop(index=1)
    return o
W = accessory([part("sphere", (sx*0.13, 0.15, 0.66), (0.13, 0.025, 0.08), "Bone", "x", 12, 6,
                    rot=(0, sx*math.radians(-25), 0)) for sx in (1, -1)], "SM_Angel_Wings", (0, 0.12, 0.60))
H = accessory([part("torus", (0, 0, 1.30), (0.16, 0.16, 0.16), "Yellow", "x", 24, 6)], "SM_Angel_Halo", (0, 0, 1.30))

# exports
def sel(objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
sel([A, B])
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "SK_Angel.fbx"), use_selection=True, object_types={'ARMATURE', 'MESH'},
                         add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', path_mode='STRIP', embed_textures=False)
for o in (W, H):
    loc = o.location.copy(); o.location = (0, 0, 0); sel([o])
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, o.name + ".fbx"), use_selection=True, object_types={'MESH'},
                             mesh_smooth_type='FACE', path_mode='STRIP')
    o.location = loc
sel([A, B, W, H])
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "angel.glb"), use_selection=True, export_format='GLB')
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "angel.blend"))

# preview renders (Cycles CPU; Workbench/Eevee abort headless; Debian build has no OIDN)
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 32; sc.cycles.use_denoising = False
sc.view_settings.view_transform = 'Standard'
sc.render.resolution_x = sc.render.resolution_y = 512
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (0.55, 0.65, 0.85, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.8
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.data.energy = 3; sun.rotation_euler = (math.radians(50), 0, math.radians(-30)); sc.collection.objects.link(sun)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 1.55
for n, ang in (("angel_front", 0), ("angel_three_quarter", 35), ("angel_back", 180)):
    a = math.radians(ang); cam.location = (3*math.sin(a), -3*math.cos(a), 0.68)
    cam.rotation_euler = (math.radians(90), 0, a)
    sc.render.filepath = os.path.join(OUT, n + ".png"); bpy.ops.render.render(write_still=True)
print("TRIS", sum(len(p.vertices) - 2 for p in B.data.polygons), sum(len(p.vertices)-2 for o in (W, H) for p in o.data.polygons))
print("BONES", [b.name for b in A.data.bones])
