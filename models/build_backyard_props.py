"""Backyard props for Take Two: Afterlife. Metres in Blender (FBX apply_unit_scale -> cm in UE).
Each prop = one mesh, origin at base centre, +Z up. Run: blender -b --factory-startup --python-exit-code 1 --python build_backyard_props.py -- <outdir>"""
import bpy, bmesh, math, os, sys
out = os.path.abspath(sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else ".")
props_dir = os.path.join(out, "Assets", "Props"); os.makedirs(props_dir, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
M = {}
def mat(n, c):
    if n not in M:
        m = bpy.data.materials.new(n); m.use_nodes = True
        b = m.node_tree.nodes["Principled BSDF"]; b.inputs["Base Color"].default_value = (*c, 1); b.inputs["Roughness"].default_value = .8
        m.diffuse_color = (*c, 1); M[n] = m
    return M[n]
parts = []
def fin(o, m):
    o.data.materials.append(m); bpy.ops.object.transform_apply(location=False, rotation=True, scale=True); parts.append(o); return o
def box(s, l, m, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=l, rotation=rot); o = bpy.context.object; o.scale = s; return fin(o, m)
def cyl(r, d, l, m, v=12, rot=(0, 0, 0), r2=None):
    if r2 is None: bpy.ops.mesh.primitive_cylinder_add(vertices=v, radius=r, depth=d, location=l, rotation=rot)
    else: bpy.ops.mesh.primitive_cone_add(vertices=v, radius1=r, radius2=r2, depth=d, location=l, rotation=rot)
    return fin(bpy.context.object, m)
def torus(R, r, z, m, a=24, b=8):
    bpy.ops.mesh.primitive_torus_add(major_radius=R, minor_radius=r, major_segments=a, minor_segments=b, location=(0, 0, z)); return fin(bpy.context.object, m)
def sphere(r, l, m, sc=(1, 1, 1), seg=12, ring=7):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=ring, radius=r, location=l); o = bpy.context.object; o.scale = sc; return fin(o, m)
def join(name):
    global parts
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts: p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]; bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)  # origin stays at world 0 = base centre
    for p in o.data.polygons: p.use_smooth = False
    parts = []; return o

# Trampoline: 3.0 m dia, 0.9 m tall
for i in range(6):
    a = i * math.tau / 6; cyl(.025, .85, (1.42 * math.cos(a), 1.42 * math.sin(a), .425), mat("Steel", (.55, .57, .6)), v=8)
torus(1.42, .03, .82, mat("Steel", (0, 0, 0)), 24, 6)
cyl(1.3, .02, (0, 0, .83), mat("BedBlack", (.04, .04, .05)), v=24)
torus(1.4, .1, .82, mat("PadBlue", (.1, .35, .85)), 24, 6)  # pad: outer r 1.5, top .92 -> squash below
pad = parts[-1]; pad.scale.z = .8; bpy.context.view_layer.objects.active = pad; pad.select_set(True)
bpy.ops.object.transform_apply(scale=True)  # top = .82+.08 = .90
tramp = join("SM_Backyard_Trampoline")

# Kiddie pool: 1.5 m dia, 0.3 m tall
pink, teal = mat("PoolPink", (.95, .45, .65)), mat("PoolTeal", (.2, .75, .75))
torus(.65, .075, .075, pink, 24, 8); torus(.65, .075, .225, teal, 24, 8)  # outer r .725
cyl(.66, .02, (0, 0, .01), pink, v=24)
cyl(.6, .02, (0, 0, .16), mat("Water", (.35, .7, 1.0)), v=24)
for p in parts[:2]:
    p.scale = (1.035, 1.035, 1); bpy.context.view_layer.objects.active = p
    bpy.ops.object.select_all(action="DESELECT"); p.select_set(True); bpy.ops.object.transform_apply(scale=True)  # outer r .75
pool = join("SM_Backyard_KiddiePool")

# Garden shed: 2.5 (X) x 2.0 (Y) x 2.3 m, door opening 0.9 x 1.75 in front (-Y) wall
wood, trim, roofm = mat("ShedWood", (.55, .35, .2)), mat("ShedTrim", (.92, .9, .82)), mat("ShedRoof", (.25, .3, .28))
W, D, H, t = 2.4, 1.9, 1.8, .06
box((W, D, .06), (0, 0, .03), trim)
box((W, t, H), (0, D / 2 - t / 2, H / 2), wood)                       # back
box((t, D - 2 * t, H), (-W / 2 + t / 2, 0, H / 2), wood); box((t, D - 2 * t, H), (W / 2 - t / 2, 0, H / 2), wood)
dw, dh = .9, 1.75; side = (W - dw) / 2
box((side, t, H), (-W / 2 + side / 2, -D / 2 + t / 2, H / 2), wood); box((side, t, H), (W / 2 - side / 2, -D / 2 + t / 2, H / 2), wood)
box((dw, t, H - dh), (0, -D / 2 + t / 2, (H + dh) / 2), wood)
box((.06, .08, dh), (-dw / 2 - .03, -D / 2 - .01, dh / 2), trim); box((.06, .08, dh), (dw / 2 + .03, -D / 2 - .01, dh / 2), trim)
me = bpy.data.meshes.new("roof"); bm = bmesh.new()  # gable prism, eaves overhang to 2.5 x 2.0
pr = [(-1.25, 1.75), (1.25, 1.75), (0, 2.3)]
vs = [bm.verts.new((x, y, z)) for y in (-1.0, 1.0) for x, z in pr]
bm.faces.new(vs[2::-1]); bm.faces.new(vs[3:])
for i in range(3): j = (i + 1) % 3; bm.faces.new((vs[i], vs[j], vs[j + 3], vs[i + 3]))
bm.normal_update(); bmesh.ops.recalc_face_normals(bm, faces=bm.faces); bm.to_mesh(me)
ro = bpy.data.objects.new("roof", me); bpy.context.collection.objects.link(ro); ro.data.materials.append(roofm); parts.append(ro)
shed = join("SM_Backyard_Shed")

# Barbecue grill (kettle): ~1.0 m
blk, steel = mat("GrillBlack", (.06, .06, .07)), mat("Steel", (0, 0, 0))
for i in range(3):
    a = i * math.tau / 3 + math.pi / 2
    cyl(.018, .7, (.2 * math.cos(a), .2 * math.sin(a), .35), steel, v=6)
for i in (-1, 1): cyl(.07, .03, (.23 * i, -.12, .07), blk, v=10, rot=(0, math.pi / 2, 0))
cyl(.23, .04, (0, .02, .38), steel, v=6)  # lower tray
cyl(.32, .22, (0, 0, .76), blk, v=16, r2=.2)  # bowl, flipped below
bowl = parts[-1]; bowl.rotation_euler.x = math.pi; bpy.ops.object.select_all(action="DESELECT"); bowl.select_set(True)
bpy.context.view_layer.objects.active = bowl; bpy.ops.object.transform_apply(rotation=True)
sphere(.32, (0, 0, .87), mat("GrillRed", (.75, .1, .08)), sc=(1, 1, .32), seg=16, ring=5)  # lid dome; lower half hidden in bowl
box((.12, .025, .025), (0, 0, .985), steel)
box((.3, .22, .025), (.5, 0, .7), mat("ShedWood", (0, 0, 0)))  # side shelf
cyl(.015, .25, (.37, 0, .7), steel, v=6, rot=(0, math.pi / 2, 0))
grill = join("SM_Backyard_Grill")
# shift so pivot is at footprint centre in X (shelf makes it asymmetric)
grill.data.transform(__import__("mathutils").Matrix.Translation((-(max(v.co.x for v in grill.data.vertices) + min(v.co.x for v in grill.data.vertices)) / 2, 0, 0)))

# Picnic table: 1.8 (X) x 1.5 (Y) x 0.75 m
pw = mat("PicnicWood", (.75, .5, .28))
for k in range(5): box((1.8, .14, .045), (0, -.3 + k * .15, .7275), pw)
for s in (-1, 1): box((1.8, .26, .045), (0, s * .62, .4375), pw)
for x in (-.65, .65):
    box((.08, 1.5, .08), (x, 0, .375), pw)              # bench support across
    box((.08, .7, .06), (x, 0, .675), pw)               # top cleat
    for s in (-1, 1):
        ang = math.atan2(.48, .705); L = math.hypot(.48, .705)
        box((.07, .08, L), (x, s * .24, .3525), pw, rot=(s * ang, 0, 0))
table = join("SM_Backyard_PicnicTable")
mn = min(v.co.z for v in table.data.vertices); table.data.transform(__import__("mathutils").Matrix.Translation((0, 0, -mn)))

# Garden gnome: 0.4 m
box((.06, .08, .03), (-.04, -.01, .015), mat("GnomeBoot", (.25, .15, .08))); box((.06, .08, .03), (.04, -.01, .015), M["GnomeBoot"])
cyl(.1, .17, (0, 0, .115), mat("GnomeBlue", (.15, .3, .7)), v=10, r2=.065)
cyl(.07, .12, (0, -.035, .2), mat("GnomeBeard", (.95, .95, .95)), v=8, r2=.0)
bb = parts[-1]; bb.rotation_euler = (math.pi, 0, 0); bpy.ops.object.select_all(action="DESELECT"); bb.select_set(True)
bpy.context.view_layer.objects.active = bb; bpy.ops.object.transform_apply(rotation=True)
sphere(.055, (0, 0, .255), mat("GnomeSkin", (.95, .72, .58)), seg=10, ring=6)
sphere(.016, (0, -.055, .25), mat("GnomeSkin", (0, 0, 0)), seg=6, ring=4)
cyl(.068, .13, (0, .005, .335), mat("GnomeRed", (.85, .1, .1)), v=10, r2=.0)
gnome = join("SM_Backyard_Gnome")

objs = [tramp, pool, shed, grill, table, gnome]
xs = [-4.2, -1.6, 0.6, 2.7, 4.5, 5.9]
for o in objs:
    bpy.ops.object.select_all(action="DESELECT"); o.select_set(True); bpy.context.view_layer.objects.active = o
    o.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=f"{props_dir}/{o.name}.fbx", use_selection=True, apply_unit_scale=True,
                             mesh_smooth_type="FACE", axis_forward="-Y", axis_up="Z")
    d = o.dimensions; tri = sum(len(p.vertices) - 2 for p in o.data.polygons)
    print(f"BUILT {o.name} {d.x*100:.1f}x{d.y*100:.1f}x{d.z*100:.1f} cm tris={tri}")
for o, x in zip(objs, xs): o.location.x = x
bpy.ops.object.select_all(action="DESELECT")
bpy.ops.export_scene.gltf(filepath=f"{out}/backyard_props.glb", export_format="GLB", export_apply=True)
bpy.ops.wm.save_as_mainfile(filepath=f"{out}/backyard_props.blend", compress=True)

sc = bpy.context.scene; sc.render.engine = "CYCLES"; sc.cycles.samples = 32; sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y = 800, 450
sc.view_settings.view_transform = "Standard"; sc.view_settings.look = "Medium High Contrast"
w = bpy.data.worlds.new("W"); w.use_nodes = True; w.node_tree.nodes["Background"].inputs[0].default_value = (.6, .75, .9, 1)
w.node_tree.nodes["Background"].inputs[1].default_value = .35; sc.world = w
bpy.ops.mesh.primitive_plane_add(size=40); bpy.context.object.data.materials.append(mat("Grass", (.3, .6, .25)))
bpy.ops.object.light_add(type="SUN", rotation=(.8, .2, .6)); bpy.context.object.data.energy = 2.5
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
for nm, loc, rot, ln in [("backyard_props_overview", (0.8, -11, 4.2), (math.radians(75), 0, 0), 32),
                         ("backyard_props_closeup", (4.6, -3.2, 1.3), (math.radians(78), 0, math.radians(8)), 35)]:
    cam.location, cam.rotation_euler = loc, rot; cam.data.lens = ln
    sc.render.filepath = f"{out}/renders/{nm}.png"; bpy.ops.render.render(write_still=True)
print("DONE")
