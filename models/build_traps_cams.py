
# Build trap + camera props for Take Two: Afterlife. Blender 3.4, run: blender -b --factory-startup --python-exit-code 1 --python build_traps_cams.py -- <outdir>
import bpy, sys, os, math
from mathutils import Vector, Matrix
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trap_fit
out = os.path.abspath(sys.argv[sys.argv.index('--')+1]) if '--' in sys.argv else os.path.abspath('out')
fbxdir = os.path.join(out, 'Assets', 'Traps'); os.makedirs(fbxdir, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
MATS = {}
def mat(name, rgb):
    if name in MATS: return MATS[name]
    m = bpy.data.materials.new('M_' + name); m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']; b.inputs['Base Color'].default_value = (*rgb, 1); b.inputs['Roughness'].default_value = 0.8
    m.diffuse_color = (*rgb, 1); MATS[name] = m; return m
PALETTE = trap_fit.palette()   # palette.md is the only colour source
_ALIAS = dict(wood='Wood', darkwood='Wood', metal='Bone', black='Ink', red='Red', pad='Orange', yellow='Yellow',
              brown='Wood', blue='Blue', green='Grass', purple='Purple', white='Bone', glass='Blue', teal='Teal',
              ink='Ink', bone='Bone', orange='Orange', sky='Sky', pink='Pink')
C = {k: PALETTE[v] for k, v in _ALIAS.items()}
parts = []
def fin(o, c):
    o.data.materials.append(mat(c, C[c])); parts.append(o); return o
def cyl(r, d, loc, rot=(0,0,0), c='wood', v=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=v, radius=r, depth=d, location=loc, rotation=rot); return fin(bpy.context.object, c)
def box(sz, loc, c='wood', rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot); o = bpy.context.object; o.scale = sz; return fin(o, c)
def sph(r, loc, c='black', sc_=(1,1,1), seg=12, ring=8, rot=(0,0,0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=ring, radius=r, location=loc, rotation=rot); o = bpy.context.object; o.scale = sc_; return fin(o, c)
def tor(R, r, loc, rot=(0,0,0), c='metal'):
    bpy.ops.mesh.primitive_torus_add(major_radius=R, minor_radius=r, major_segments=12, minor_segments=6, location=loc, rotation=rot); return fin(bpy.context.object, c)
def prism(yz, x0, x1, c):
    """Extrude a convex YZ polygon from x0 to x1 (used for slanted hazard stripes)."""
    import bmesh
    me = bpy.data.meshes.new('prism'); bm = bmesh.new()
    a = [bm.verts.new((x0, y, z)) for y, z in yz]; b = [bm.verts.new((x1, y, z)) for y, z in yz]
    bm.faces.new(a[::-1]); bm.faces.new(b); n = len(yz)
    for i in range(n): bm.faces.new((a[i], a[(i+1) % n], b[(i+1) % n], b[i]))
    bm.to_mesh(me); bm.free(); o = bpy.data.objects.new('prism', me); sc.collection.objects.link(o); return fin(o, c)
def cone(r1, r2, d, loc, rot=(0,0,0), c='metal', v=8):
    bpy.ops.mesh.primitive_cone_add(vertices=v, radius1=r1, radius2=r2, depth=d, location=loc, rotation=rot); return fin(bpy.context.object, c)
def done(name):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts: p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join(); o = bpy.context.object; o.name = o.data.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)  # origin = world origin = pivot
    parts = []; return o
X90 = (0, math.pi/2, 0); Y90 = (math.pi/2, 0, 0)   # cylinder axis -> X ; -> Y
items = []
# --- Hammer (procedural fallback): spiked mallet, pivot at the top of the handle (hinge), hangs down, swings around X
cyl(0.07, 0.20, (0,0,0), X90, 'metal')                              # hinge pin along X
cyl(0.05, 0.10, (0,0,-0.05), c='ink', v=10)                         # hinge collar
cyl(0.04, 0.58, (0,0,-0.39), c='wood', v=10)                        # handle
for z in (-0.20, -0.45): cyl(0.046, 0.03, (0,0,z), c='red', v=10)   # grip bands
cyl(0.14, 0.38, (0,0,-0.77), Y90, 'wood', 14)                       # mallet head along Y (strike direction)
for y in (-0.19, 0.19): cyl(0.142, 0.03, (0,y,-0.77), Y90, 'ink', 14)  # iron rings
for sy in (-1, 1):                                                  # spikes on both strike faces
    cone(0.035, 0.0, 0.06, (0, sy*0.22, -0.77), (sy*-math.pi/2, 0, 0), 'metal', 6)
    for k in range(4):
        a = k*math.pi/2 + math.pi/4
        cone(0.03, 0.0, 0.06, (0.08*math.cos(a), sy*0.22, -0.77 + 0.08*math.sin(a)), (sy*-math.pi/2, 0, 0), 'metal', 6)
items.append(done('SM_Trap_Hammer'))
# --- Cannon: pivot base centre, barrel +X
box((0.62,0.44,0.14), (0,0,0.25), 'darkwood')
for x in (-0.2, 0.2):
    for y in (-0.27, 0.27): cyl(0.18, 0.08, (x,y,0.18), Y90, 'wood', 12)
cyl(0.16, 0.72, (0.08,0,0.48), X90, 'black', 14)
cyl(0.20, 0.08, (0.42,0,0.48), X90, 'black', 14)
sph(0.16, (-0.28,0,0.48), 'black')
items.append(done('SM_Trap_Cannon'))
sph(0.5, (0,0,0), 'black', seg=16, ring=10); items.append(done('SM_Trap_CannonBall'))
# --- Spinner (procedural fallback): bladed hub, pivot at hub base centre, spins around Z
cyl(0.22, 0.06, (0,0,0.03), c='ink', v=16)                          # base plate
for k in range(6): cyl(0.015, 0.012, (0.17*math.cos(k*math.pi/3), 0.17*math.sin(k*math.pi/3), 0.066), c='metal', v=6)  # bolts
cyl(0.07, 0.36, (0,0,0.24), c='metal', v=12)                        # shaft
cyl(0.12, 0.12, (0,0,0.44), c='red', v=14)                          # hub
cone(0.12, 0.04, 0.10, (0,0,0.55), c='red', v=14)                   # hub cap
for sx in (-1, 1):                                                  # two arms, each ending in a blade
    box((0.30,0.06,0.05), (sx*0.21,0,0.44), 'metal')
    prism([(-0.20,0.36),(0.20,0.36),(0.12,0.52),(-0.12,0.52)], sx*0.36, sx*0.50, 'yellow')   # blade, tapered
    prism([(-0.20,0.355),(0.20,0.355),(0.20,0.365),(-0.20,0.365)], sx*0.36, sx*0.50, 'ink')   # blade edge band
items.append(done('SM_Trap_Spinner'))
# --- PusherWall: 100 wide (Y) x 20 deep (X) x 100 tall, pivot base centre, pushes +X
box((0.08,1.0,1.0), (-0.06,0,0.5), 'wood')                          # backing board
for y in (-0.44, 0.44): box((0.04,0.06,0.9), (-0.08,y,0.5), 'ink')  # back rails
pads = [(-0.27,0.73), (0.27,0.73), (-0.27,0.27), (0.27,0.27)]       # 2x2 cushions
for y, z in pads:
    box((0.10,0.40,0.36), (0.03,y,z), 'pad')
    box((0.02,0.36,0.32), (0.09,y,z), 'pad')                        # puffed face
for z in (0.04, 0.96):                                              # hazard stripe bands top and bottom, slanted
    box((0.12,1.0,0.08), (0.0,0,z), 'ink')
    for k in range(10):
        y0 = -0.5 + k*0.1
        poly = [(max(-0.5,y0),z-0.04),(min(0.5,y0+0.05),z-0.04),(min(0.5,y0+0.09),z+0.04),(max(-0.5,y0+0.04),z+0.04)]
        prism(poly, 0.06, 0.102, 'yellow')
items.append(done('SM_Trap_PusherWall'))
# --- Trapdoor: hinge edge at -X, z=0
box((1.0,1.0,0.07), (0.5,0,0.035), 'wood')
for y in (-0.3, 0.3): box((0.96,0.08,0.01), (0.5,y,0.075), 'darkwood')
for y in (-0.3, 0.3): cyl(0.03, 0.12, (0.03,y,0.04), Y90, 'metal')   # hinge knuckles
tor(0.07, 0.015, (0.82,0,0.088), c='metal')
items.append(done('SM_Trap_Trapdoor'))
# --- Banana peel ~60x40x15, base centre
sph(1, (0,0,0.06), 'yellow', sc_=(0.13,0.07,0.06))
for a in (25, 155, 205, 335):
    t = math.radians(a); d = 0.15
    sph(1, (d*math.cos(t), d*math.sin(t), 0.02), 'yellow', sc_=(0.15,0.07,0.02), rot=(0,0,t))
    sph(0.02, (0.285*math.cos(t), 0.285*math.sin(t)*0.95, 0.02), 'brown', seg=8, ring=5)
cyl(0.025, 0.06, (0,0,0.12), c='brown', v=8)
items.append(done('SM_Trap_Banana'))
# --- Lawnmower ~100x60x50, pivot centre of deck base, drives +X (procedural: no CC0 model found)
box((0.80,0.50,0.10), (0.02,0,0.09), 'green')                       # deck (base at z=0.04)
cyl(0.24, 0.08, (0.02,0,0.15), c='green', v=16)                     # rounded deck dome
box((0.05,0.52,0.07), (0.45,0,0.09), 'ink')                         # front bumper
for x in (-0.28, 0.30):
    for y in (-0.27, 0.27):
        cyl(0.09, 0.06, (x,y,0.09), Y90, 'ink', 12)                 # tyre
        cyl(0.05, 0.062, (x,y,0.09), Y90, 'metal', 8)               # hub
box((0.24,0.22,0.12), (0.06,0,0.25), 'red')                         # engine
cyl(0.05, 0.05, (0.06,0,0.335), c='ink', v=10)                      # fuel cap
box((0.14,0.10,0.10), (-0.16,0,0.22), 'ink')                        # grass bag stub
for y in (-0.19, 0.19): box((0.40,0.03,0.03), (-0.31,y,0.30), 'metal', rot=(0, math.atan2(0.30,0.33), 0))  # handle arms
box((0.04,0.42,0.04), (-0.48,0,0.48), 'ink')                        # handle bar
box((0.03,0.14,0.035), (-0.47,0,0.48), 'red')                       # grip
items.append(done('SM_Trap_Lawnmower'))
# Blade disc: separate mesh so UE can spin it (visual only). Pivot = its own centre; place at (0,0,0.03) under the deck.
cyl(0.20, 0.012, (0,0,0), c='metal', v=16)
for k in range(2): box((0.40,0.05,0.016), (0,0,0), 'ink', rot=(0,0,k*math.pi/2))
items.append(done('SM_Trap_LawnmowerBlade'))
# --- Cameras, pivot at the body bottom. No tripod: they are placed in the air, so legs just dangle.
def cambody(z, c):
    box((0.16,0.09,0.10), (0,0,z), c); cyl(0.035, 0.07, (0.11,0,z), X90, 'black', 10)
    cyl(0.025, 0.005, (0.146,0,z), X90, 'glass', 10)
cambody(0.05, 'red'); items.append(done('SM_Cam_Fixed'))
cyl(0.045, 0.03, (0,0,0.015), c='metal', v=12); box((0.14,0.015,0.015), (-0.12,0,0.015), 'black')
cambody(0.08, 'blue'); items.append(done('SM_Cam_Pan'))
# drone
for y in (-0.07, 0.07): box((0.22,0.015,0.015), (0,y,0.0075), 'black'); box((0.015,0.015,0.06), (0.06,y,0.045), 'black'); box((0.015,0.015,0.06), (-0.06,y,0.045), 'black')
box((0.18,0.14,0.07), (0,0,0.11), 'green')
for k in range(4):
    t = math.pi/4 + k*math.pi/2
    box((0.20,0.025,0.02), (0.12*math.cos(t), 0.12*math.sin(t), 0.13), 'black', rot=(0,0,t))
    cyl(0.012, 0.03, (0.2*math.cos(t), 0.2*math.sin(t), 0.15), c='black', v=8)
    cyl(0.07, 0.006, (0.2*math.cos(t), 0.2*math.sin(t), 0.168), c='white', v=12)
sph(0.035, (0.07,0,0.07), 'black', seg=10, ring=6); cyl(0.015, 0.01, (0.105,0,0.07), X90, 'glass', 8)
items.append(done('SM_Cam_Follow'))
# CCTV dome (sits on its mount plate; flip in UE for ceiling use)
cyl(0.20, 0.04, (0,0,0.02), c='purple', v=16)
sph(0.16, (0,0,0.16), 'glass', sc_=(1,1,0.9), seg=16, ring=8)
cyl(0.165, 0.03, (0,0,0.17), c='white', v=16)
items.append(done('SM_Cam_TrapCam'))

# --- swap in the CC0 downloads (trap_fit.THIRDPARTY); a missing file keeps the primitive built above
BOUNDS = trap_fit.load_bounds(); PAL = trap_fit.palette()
def imported(name, fname, colour):
    path = os.path.join(trap_fit.THIRDPARTY_DIR, fname)
    if not os.path.exists(path):
        print('FALLBACK', name, 'primitive, missing', path); return None
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    obs = [o for o in new if o.type == 'MESH']
    if not obs: raise RuntimeError('no mesh in ' + path)
    for o in obs:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    for o in new:
        if o.type != 'MESH': bpy.data.objects.remove(o)
    bpy.ops.object.select_all(action='DESELECT')
    for o in obs: o.select_set(True)
    bpy.context.view_layer.objects.active = obs[0]
    bpy.ops.object.join(); o = bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vs = [v.co for v in o.data.vertices]
    mn = [min(v[i] for v in vs) * 100 for i in range(3)]; mx = [max(v[i] for v in vs) * 100 for i in range(3)]
    rows = trap_fit.fit_matrix(mn, mx, BOUNDS[name])   # cm -> cm: motion axes, pivot at origin, recorded box
    o.data.transform(Matrix.Scale(0.01, 4) @ Matrix([*rows, [0, 0, 0, 1]]) @ Matrix.Scale(100, 4)); o.data.update()
    o.data.materials.clear(); o.data.materials.append(mat('pal_' + colour, PAL[colour]))
    o.name = o.data.name = name; print('IMPORTED', name, fname); return o
for name, (fname, _url, colour) in trap_fit.THIRDPARTY.items():
    k = [p.name for p in items].index(name); prim = items[k]; prim.name = prim.data.name = name + '_primitive'
    o = imported(name, fname, colour)
    if o: bpy.data.objects.remove(prim); items[k] = o
    else: prim.name = prim.data.name = name
# --- export each FBX at origin
for o in items:
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.fbx(filepath=os.path.join(fbxdir, o.name + '.fbx'), use_selection=True, apply_scale_options='FBX_SCALE_UNITS',
        mesh_smooth_type='FACE', add_leaf_bones=False, object_types={'MESH'}, use_mesh_modifiers=True)
    me = o.data; me.calc_loop_triangles(); print('TRIS', o.name, len(me.loop_triangles))
# --- layout for preview / blend / glb
for i, o in enumerate(items):
    o.location = ((i % 6) * 1.5, -(i // 6) * 1.6, 0)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=os.path.join(out, 'traps_cams.glb'), use_selection=True)
w = bpy.data.worlds.new('W'); sc.world = w; w.use_nodes = True; w.node_tree.nodes['Background'].inputs[0].default_value = (0.6,0.75,0.9,1); w.node_tree.nodes['Background'].inputs[1].default_value = 0.35
bpy.ops.object.light_add(type='SUN', rotation=(0.8,0.2,0.6)); bpy.context.object.data.energy = 2.5
bpy.ops.mesh.primitive_plane_add(size=30, location=(3.75,-0.8,-0.001)); bpy.context.object.data.materials.append(mat('floor', (0.45,0.5,0.45)))
bpy.ops.object.camera_add(location=(3.75,-9.5,5.5), rotation=(math.radians(58),0,0)); cam = bpy.context.object; sc.camera = cam; cam.data.lens = 38
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 32; sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y = 800, 450
sc.view_settings.view_transform = 'Standard'; sc.view_settings.look = 'Medium High Contrast'
sc.render.filepath = os.path.join(out, 'traps_cams_preview.png'); bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, 'traps_cams.blend'), compress=True)
print('DONE')
