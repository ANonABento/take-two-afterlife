
# Build trap + camera props for Take Two: Afterlife. Blender 3.4, run: blender -b --factory-startup --python-exit-code 1 --python build_traps_cams.py -- <outdir>
import bpy, sys, os, math
from mathutils import Vector
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
C = dict(wood=(0.55,0.32,0.14), darkwood=(0.30,0.16,0.06), metal=(0.35,0.37,0.40), black=(0.04,0.04,0.05),
         red=(0.85,0.12,0.10), pad=(0.95,0.45,0.10), yellow=(0.98,0.85,0.15), brown=(0.35,0.22,0.08),
         blue=(0.15,0.40,0.90), green=(0.15,0.75,0.30), purple=(0.55,0.25,0.85), white=(0.92,0.92,0.92),
         glass=(0.10,0.15,0.25), teal=(0.1,0.7,0.7))
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
# --- Hammer: pivot at hinge (top of post), hangs down, swings around X
cyl(0.07, 0.20, (0,0,0), X90, 'metal')             # hinge pin along X
cyl(0.045, 0.66, (0,0,-0.33), c='wood')            # post / handle
cyl(0.14, 0.50, (0,0,-0.77), Y90, 'wood', 14)      # mallet head along Y (strike direction)
cyl(0.145, 0.05, (0, 0.22,-0.77), Y90, 'darkwood', 14); cyl(0.145, 0.05, (0,-0.22,-0.77), Y90, 'darkwood', 14)
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
# --- Spinner: pivot hub base centre, spins around Z
cyl(0.22, 0.06, (0,0,0.03), c='metal', v=16)
cyl(0.11, 0.50, (0,0,0.31), c='red', v=14)
sph(0.12, (0,0,0.56), 'red', sc_=(1,1,0.5))
box((0.64,0.10,0.10), (0,0,0.42), 'metal')
box((0.18,0.20,0.22), (0.41,0,0.42), 'pad'); box((0.18,0.20,0.22), (-0.41,0,0.42), 'pad')
items.append(done('SM_Trap_Spinner'))
# --- PusherWall: 100 wide (Y) x 20 deep (X) x 100 tall, faces +X
box((0.14,1.0,1.0), (-0.03,0,0.5), 'wood')
box((0.06,0.88,0.88), (0.07,0,0.5), 'pad')
box((0.062,0.88,0.08), (0.071,0,0.5), 'red')
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
# --- Cameras (~40 cm), pivot base centre
def tripod(hc):
    for k in range(3):
        t = k*2*math.pi/3 + math.pi/6
        cyl(0.012, 0.27, (0.055*math.cos(t), 0.055*math.sin(t), 0.135), (0, -0.4, t), 'black', 6)
    cyl(0.02, 0.06, (0,0,0.27), c='black', v=8)
def cambody(z, c):
    box((0.16,0.09,0.10), (0,0,z), c); cyl(0.035, 0.07, (0.11,0,z), X90, 'black', 10)
    cyl(0.025, 0.005, (0.146,0,z), X90, 'glass', 10)
tripod('red'); cambody(0.35, 'red'); items.append(done('SM_Cam_Fixed'))
tripod('blue'); cyl(0.045, 0.03, (0,0,0.315), c='metal', v=12); box((0.14,0.015,0.015), (-0.12,0,0.315), 'black')
cambody(0.38, 'blue'); items.append(done('SM_Cam_Pan'))
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
