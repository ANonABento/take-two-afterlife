"""Backyard course kit (UE5.5 jam, Run course). Blender 3.4 headless.
blender -b --factory-startup --python-exit-code 1 --python build_course_kit.py -- OUTDIR [render]
Units: Blender metres (1 BU = 100 cm). FBX re-import verified in cm.
"""
import bpy, bmesh, sys, os, math, json
from mathutils import Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = os.path.abspath(argv[0]); RENDER = "render" in argv
FBX_DIR = os.path.join(OUT, "Assets", "Course"); os.makedirs(FBX_DIR, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
cm = 0.01
PAL = {"Dirt": (0.55, 0.38, 0.22), "Stone": (0.62, 0.62, 0.6), "Grass": (0.30, 0.62, 0.25),
       "Wood": (0.62, 0.42, 0.24), "WoodDark": (0.38, 0.24, 0.13), "Red": (0.85, 0.16, 0.14),
       "Black": (0.06, 0.06, 0.07), "White": (0.93, 0.93, 0.9), "Gold": (0.92, 0.72, 0.18),
       "Checker": (0.12, 0.12, 0.12)}
MATS = {}
for n, c in PAL.items():
    m = bpy.data.materials.new("M_Course_" + n); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*c, 1); b.inputs["Roughness"].default_value = 0.85
    m.diffuse_color = (*c, 1); MATS[n] = m

def box(name, x0, x1, y0, y1, z0, z1, mat):
    me = bpy.data.meshes.new(name); bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, verts=bm.verts, matrix=Matrix.Diagonal(((x1-x0)*cm, (y1-y0)*cm, (z1-z0)*cm, 1)))
    bmesh.ops.translate(bm, verts=bm.verts, vec=((x0+x1)/2*cm, (y0+y1)/2*cm, (z0+z1)/2*cm))
    bm.to_mesh(me); bm.free(); me.materials.append(MATS[mat])
    o = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(o); return o

def cyl_y(name, cx, cy, cz, r, depth, mat, seg=16):
    """Cylinder with axis along Y."""
    me = bpy.data.meshes.new(name); bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=seg, radius1=r*cm, radius2=r*cm, depth=depth*cm)
    bmesh.ops.rotate(bm, verts=bm.verts, matrix=Matrix.Rotation(math.pi/2, 3, 'X'))
    bmesh.ops.translate(bm, verts=bm.verts, vec=(cx*cm, cy*cm, cz*cm))
    bm.to_mesh(me); bm.free(); me.materials.append(MATS[mat])
    o = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(o); return o

def wedge(name, x0, x1, y0, y1, h, mat):
    """Ramp: z=0 at x0, rising to h at x1."""
    v = [(x0,y0,0),(x1,y0,0),(x1,y1,0),(x0,y1,0),(x1,y0,h),(x1,y1,h)]
    v = [(a*cm,b*cm,c*cm) for a,b,c in v]
    f = [(0,3,2,1),(1,2,5,4),(0,1,4),(3,5,2),(0,4,5,3)]
    me = bpy.data.meshes.new(name); me.from_pydata(v, [], f); me.update()
    me.materials.append(MATS[mat])
    o = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(o); return o

def finish(name, parts):
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts: p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    o = bpy.context.active_object; o.name = name; o.data.name = name
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.remove_doubles(threshold=0.0001); bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    o.location = (0, 0, 0); o.rotation_euler = (0, 0, 0); o.scale = (1, 1, 1)
    return o

pieces = {}
# 1 Straight 400 x 200 x 10: dirt path with stone kerbs along long edges
pieces["SM_Course_Straight"] = finish("SM_Course_Straight", [
    box("a", -200, 200, -90, 90, 0, 10, "Dirt"),
    box("b", -200, 200, -100, -90, 0, 10, "Stone"),
    box("c", -200, 200, 90, 100, 0, 10, "Stone")])
# 2 Corner 200 x 200 x 10: path turns from -X to +Y; kerbs on outer (+X, -Y) sides, post at inner corner
pieces["SM_Course_Corner90"] = finish("SM_Course_Corner90", [
    box("a", -100, 90, -90, 100, 0, 10, "Dirt"),
    box("b", -100, 100, -100, -90, 0, 10, "Stone"),
    box("c", 90, 100, -90, 100, 0, 10, "Stone")])
# 3 Ramp 400 x 200, rising 0 -> 100 cm along +X, kerbs on sides
pieces["SM_Course_Ramp"] = finish("SM_Course_Ramp", [
    wedge("a", -200, 200, -90, 90, 100, "Dirt"),
    wedge("b", -200, 200, -100, -90, 100, "Stone"),
    wedge("c", -200, 200, 90, 100, 100, "Stone")])
# 4 Plank bridge 400 x 100, 50 tall: posts, beams, 8 planks with gaps
br = []
for x in (-195, -5, 185):
    for y in (-50, 40):
        br.append(box("p", x, x+10, y, y+10, 0, 40, "WoodDark"))
br += [box("bm", -200, 200, -48, -40, 32, 40, "WoodDark"), box("bm2", -200, 200, 40, 48, 32, 40, "WoodDark")]
for i in range(8):
    x0 = -200 + i*50
    br.append(box("pl", x0+1, x0+49, -50, 50, 40, 50, "Wood"))
pieces["SM_Course_PlankBridge"] = finish("SM_Course_PlankBridge", br)

def gate_frame(top=250):
    return [box("L", -150, -120, -25, 25, 0, 220, "Wood"),
            box("R", 120, 150, -25, 25, 0, 220, "Wood"),
            box("T", -150, 150, -25, 25, 220, top, "WoodDark")]
# 5 Start gate 300 x 50 x 300: arch + clapperboard on top
st = gate_frame()
st.append(box("slate", -60, 60, -6, 6, 250, 285, "Black"))
st.append(box("label", -50, 50, -7, 7, 255, 270, "White"))
for i in range(6):  # striped clapper stick
    x0 = -60 + i*20
    st.append(box("s", x0, x0+20, -6, 6, 287, 300, "White" if i % 2 == 0 else "Black"))
st.append(box("hinge", -60, -50, -8, 8, 285, 288, "Stone"))
pieces["SM_Course_GateStart"] = finish("SM_Course_GateStart", st)
# 6 Finish gate: arch + two film reels joined by a film strip, checker band
fi = gate_frame(235)
for i in range(10):
    x0 = -150 + i*30
    fi.append(box("ck", x0, x0+30, -25, 25, 235, 250, "White" if i % 2 == 0 else "Checker"))
for cx in (-45, 45):
    fi.append(cyl_y("reel", cx, 0, 275, 25, 10, "Black", 20))
    fi.append(cyl_y("hub", cx, 0, 275, 6, 14, "Gold", 12))
    for k in range(5):
        a = k * 2*math.pi/5 + math.pi/2
        fi.append(cyl_y("hole", cx + 14*math.cos(a), 0, 275 + 14*math.sin(a), 5, 12, "Stone", 8))
fi.append(box("film", -45, 45, -4, 4, 252, 262, "Black"))
pieces["SM_Course_GateFinish"] = finish("SM_Course_GateFinish", fi)

def tris(o):
    return sum(len(p.vertices) - 2 for p in o.data.polygons)

# Export FBX (one per piece) for UE5
for name, o in pieces.items():
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.fbx(filepath=os.path.join(FBX_DIR, name + ".fbx"), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
        mesh_smooth_type='FACE', add_leaf_bones=False, axis_forward='-Z', axis_up='Y',
        bake_space_transform=False, use_mesh_modifiers=True)

# Lay out for preview / GLB / .blend (export already done at origin)
xs = {"SM_Course_Straight": (-6.5, 2), "SM_Course_Corner90": (-2.5, 2), "SM_Course_Ramp": (1.5, 2),
      "SM_Course_PlankBridge": (6.0, 2), "SM_Course_GateStart": (-3, -1.5), "SM_Course_GateFinish": (3, -1.5)}

# Verify: re-import each FBX into a clean scene
report = {}
for name in pieces:
    bpy.ops.object.select_all(action='DESELECT')
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(FBX_DIR, name + ".fbx"))
    imp = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
    o = imp[0]; bpy.context.view_layer.update()
    ws = [o.matrix_world @ v.co for v in o.data.vertices]
    mn = [min(w[i] for w in ws)*100 for i in range(3)]; mx = [max(w[i] for w in ws)*100 for i in range(3)]
    dims = [round(mx[i]-mn[i], 2) for i in range(3)]
    report[name] = {"dims_cm": dims, "min_cm": [round(v, 2) for v in mn], "max_cm": [round(v, 2) for v in mx],
                    "pivot_cm": [round(v*100, 3) for v in o.matrix_world.translation],
                    "tris": tris(o), "fbx_kb": round(os.path.getsize(os.path.join(FBX_DIR, name+".fbx"))/1024, 1)}
    for x in imp: bpy.data.objects.remove(x)
print("REPORT", json.dumps(report))
with open(os.path.join(OUT, "verify_report.json"), "w") as f: json.dump(report, f, indent=1)

if RENDER:
    for name, (x, y) in xs.items(): pieces[name].location = (x, y, 0)
    gnd = box("Ground", -1100, 1100, -700, 700, -2, 0, "Grass")
    sc = bpy.context.scene
    for n, (x, y) in xs.items():
        pieces[n].select_set(True)
    bpy.ops.object.select_all(action='DESELECT')
    for n in pieces: pieces[n].select_set(True)
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, "course_kit.glb"), use_selection=True, export_format='GLB')
    sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 32
    sc.cycles.use_denoising = False; sc.render.resolution_x = 800; sc.render.resolution_y = 450
    sc.view_settings.view_transform = 'Standard'
    w = bpy.data.worlds.new("W"); w.use_nodes = True
    w.node_tree.nodes["Background"].inputs[0].default_value = (0.75, 0.85, 1, 1)
    w.node_tree.nodes["Background"].inputs[1].default_value = 0.8; sc.world = w
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN'))
    sun.data.energy = 3.5; sun.rotation_euler = (math.radians(45), 0, math.radians(30)); sc.collection.objects.link(sun)
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
    def shot(path, loc, tgt, lens):
        cam.location = loc; cam.data.lens = lens
        d = (__import__('mathutils').Vector(tgt) - cam.location)
        cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = path; bpy.ops.render.render(write_still=True)
    shot(os.path.join(OUT, "course_kit_preview.png"), (0, -15, 9), (0, 0.4, 0.6), 32)
    shot(os.path.join(OUT, "course_kit_preview_side.png"), (12, -9, 4), (0, 0.3, 0.8), 30)
    gnd.select_set(False)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "course_kit.blend"), compress=True)
