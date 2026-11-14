# v2: parts remapped for gen2 (blank-face refs); face = largest connected island
# Blender 3.4 headless: blender -b --factory-startup --python-exit-code 1 --python build_angel.py -- <raw_lite.glb> <rig_joints.json> <outdir>
import bpy, bmesh, sys, json, math, os
from mathutils import Vector, Matrix
argv = sys.argv[sys.argv.index('--')+1:]
RAW, JOINTS, OUT = [os.path.abspath(a) for a in argv]
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=RAW)
objs = {o.data.name.split('.')[0]: o for o in bpy.context.scene.objects if o.type == 'MESH'}
print('map', sorted((o.name, k) for k, o in objs.items()))
print('parts', sorted(objs))
for o in objs.values():  # bake transforms
    o.select_set(True)
bpy.context.view_layer.objects.active = list(objs.values())[0]
bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for o in [o for o in bpy.context.scene.objects if o.type == 'EMPTY']:
    bpy.data.objects.remove(o)

def wbb(o):
    vs = [o.matrix_world @ v.co for v in o.data.vertices]
    return Vector([min(v[i] for v in vs) for i in range(3)]), Vector([max(v[i] for v in vs) for i in range(3)])

# --- identify parts (from inspection of the Tripo part split) ---
HALO = 'tripo_mesh_2'; WINGS = ['tripo_mesh_3', 'tripo_mesh_6']
OUTFIT = ['tripo_mesh_1', 'tripo_mesh_8', 'tripo_mesh_12']
STRAY = ['tripo_mesh_7']  # extra wing Tripo grew on the FRONT right of the chest (x+0.11..0.20); not a bow
HEAD = 'tripo_mesh_0'
wing_bbs = [wbb(objs[w]) for w in WINGS]
halo_bb = wbb(objs[HALO])
for n in [HALO] + WINGS + STRAY:
    bpy.data.objects.remove(objs.pop(n))

# front direction: wings sit on the back -> front is opposite their X offset
body_cx = sum((wbb(o)[0].x + wbb(o)[1].x)/2 for o in objs.values())/len(objs)
wing_cx = sum((a.x+b.x)/2 for a, b in wing_bbs)/2
front_sign = -1 if wing_cx > body_cx else 1   # +1 = model faces +X
print('wing_cx', round(wing_cx, 3), 'body_cx', round(body_cx, 3), 'front +X' if front_sign > 0 else 'front -X')

# --- decimate each part to a tri budget ---
TARGET = {'tripo_mesh_0': 3800, 'tripo_mesh_1': 1400, 'tripo_mesh_12': 800, 'tripo_mesh_8': 600, 'tripo_mesh_7': 300, 'tripo_mesh_11': 250, 'tripo_mesh_13': 250,
          'tripo_mesh_4': 500, 'tripo_mesh_5': 500, 'tripo_mesh_9': 500, 'tripo_mesh_10': 500,
          }
for n, o in objs.items():
    tris = sum(len(p.vertices)-2 for p in o.data.polygons)
    t = TARGET.get(n, 300)
    if tris > t:
        m = o.modifiers.new('dec', 'DECIMATE'); m.ratio = t/tris
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.modifier_apply(modifier='dec')

# --- materials ---
def mat(name, rgba):
    m = bpy.data.materials.new(name); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = rgba
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 0.6
    m.diffuse_color = rgba; return m
M_BODY = mat('Body', (0.95, 0.95, 0.95, 1)); M_OUT = mat('Outfit', (0.85, 0.75, 0.45, 1)); M_FACE = mat('Face', (1.0, 0.85, 0.8, 1))
for n, o in objs.items():
    o.data.materials.clear()
    for m in (M_BODY, M_OUT, M_FACE): o.data.materials.append(m)
    idx = 1 if n in OUTFIT else 0
    for p in o.data.polygons: p.material_index = idx
# face region on head: front-facing polys in the face window
head = objs[HEAD]; hb0, hb1 = wbb(head)
H = hb1.z - hb0.z
fz0, fz1 = hb0.z + 0.30*H, hb0.z + 0.78*H
cy = (hb0.y+hb1.y)/2; fw = 0.30*(hb1.y-hb0.y)
nface = 0
for p in head.data.polygons:
    c = p.center
    if p.normal.x*front_sign > 0.35 and fz0 < c.z < fz1 and abs(c.y-cy) < fw:
        p.material_index = 2; nface += 1
# keep only the largest connected face island (fix: old selection was 87 islands)
import bmesh as _bm
bmh=_bm.new(); bmh.from_mesh(head.data); bmh.faces.ensure_lookup_table()
sel={f.index for f in bmh.faces if f.material_index==2}; seen=set(); isl=[]
for f0 in sel:
    if f0 in seen: continue
    st=[f0]; comp=[]; seen.add(f0)
    while st:
        f=bmh.faces[st.pop()]; comp.append(f.index)
        for e in f.edges:
            for g in e.link_faces:
                if g.index in sel and g.index not in seen: seen.add(g.index); st.append(g.index)
    isl.append(comp)
isl.sort(key=len); keep=set(isl[-1]) if isl else set()
for p in head.data.polygons:
    if p.material_index==2 and p.index not in keep: p.material_index=0
print('face islands before', len(isl), 'kept polys', len(keep))

# --- join --- (store part id per vertex)
PART_NAMES = list(objs)
for i,(n,o) in enumerate(objs.items()):
    at = o.data.attributes.new('part', 'INT', 'POINT')
    for k in range(len(o.data.vertices)): at.data[k].value = i

bpy.ops.object.select_all(action='DESELECT')
for o in objs.values(): o.select_set(True)
bpy.context.view_layer.objects.active = head
bpy.ops.object.join(); body = head; body.name = 'Angel'; body.data.name = 'Angel'
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.quads_convert_to_tris(); bpy.ops.object.mode_set(mode='OBJECT')

# --- orient: front to -Y, feet on Z=0, height 1.0 m ---
rot = Matrix.Rotation(math.radians(-90 if front_sign > 0 else 90), 4, 'Z')
b0, b1 = wbb(body); s = 1.0/(b1.z-b0.z)
ctr = Vector(((b0.x+b1.x)/2, (b0.y+b1.y)/2, b0.z))
XF = Matrix.Scale(s, 4) @ rot @ Matrix.Translation(-ctr)
body.data.transform(XF); body.matrix_world = Matrix()
print('scale', round(s, 4))
part_of = [PART_NAMES[d.value] for d in body.data.attributes['part'].data]
body.data.attributes.remove(body.data.attributes['part'])

# --- UVs: smart project, then face polys planar into 0-1 ---
bpy.context.view_layer.objects.active = body; body.select_set(True)
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
bm = bmesh.from_edit_mesh(body.data); uv = bm.loops.layers.uv.active
ff = [f for f in bm.faces if f.material_index == 2]
xs = [l.vert.co.x for f in ff for l in f.loops]; zs = [l.vert.co.z for f in ff for l in f.loops]
x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
for f in ff:
    for l in f.loops:
        l[uv].uv = ((l.vert.co.x-x0)/(x1-x0), (l.vert.co.z-z0)/(z1-z0))
bmesh.update_edit_mesh(body.data); bpy.ops.object.mode_set(mode='OBJECT')

# --- rebuilt wings: separate simple meshes ---
def make_wing(side):
    bmw = bmesh.new()
    # feather fan outline in the wing plane (local: X = out, Z = up)
    pts = [(0, 0), (0.05, 0.10), (0.13, 0.16), (0.19, 0.13), (0.17, 0.07), (0.20, 0.03), (0.15, -0.01), (0.16, -0.06), (0.09, -0.05), (0.07, -0.09), (0.02, -0.04)]
    vs = [bmw.verts.new((x, 0, z)) for x, z in pts]
    bmw.faces.new(vs); bmesh.ops.triangulate(bmw, faces=bmw.faces[:])
    ext = bmesh.ops.extrude_face_region(bmw, geom=bmw.faces[:])
    bmesh.ops.translate(bmw, verts=[e for e in ext['geom'] if isinstance(e, bmesh.types.BMVert)], vec=(0, 0.02, 0))
    bmesh.ops.recalc_face_normals(bmw, faces=bmw.faces[:])
    me = bpy.data.meshes.new('Wing_'+side); bmw.to_mesh(me); bmw.free()
    o = bpy.data.objects.new('Wing_'+side, me); bpy.context.scene.collection.objects.link(o)
    me.materials.append(M_BODY)
    return o
# place from the generated wing roots (transformed by XF)
roots = []
for a, b in wing_bbs:
    c = XF @ ((a+b)/2); roots.append(c)
wings = []
for c in roots:
    side = 'L' if c.x > 0 else 'R'
    w = make_wing(side)
    sx = 1 if side == 'L' else -1
    w.scale = (sx*1.0, 1, 1)
    w.location = (sx*0.05, c.y - 0.01, c.z - 0.0)
    w.rotation_euler = (0, 0, math.radians(sx*20))
    wings.append(w)
print('wing roots', [tuple(round(v, 3) for v in c) for c in roots])

# --- armature: Tripo's auto-rig skeleton did not fit this chibi (toes pointed backwards, shoulders at 71% height
# vs 47% on the mesh, heat weights failed), so bones are placed from the part-split landmarks (gen frame: Y up, front +X,
# character right = +Z) and each part is weighted to its own bones only (rigid-ish chibi parts).
G = {  # name: (head, tail, parent) in generated-mesh coords
 'Hips': ((0,-0.26,0),(0,-0.15,0),None), 'Spine': ((0,-0.15,0),(0,-0.07,0),'Hips'), 'Spine1': ((0,-0.07,0),(0,-0.02,0),'Spine'),
 'Spine2': ((0,-0.02,0),(0,0.01,0),'Spine1'), 'Neck': ((0,0.01,0),(0,0.04,0),'Spine2'), 'Head': ((0,0.04,0),(0,0.40,0),'Neck'),
 'LeftArm': ((0,-0.05,-0.11),(0,-0.05,-0.22),'Spine2'), 'LeftForeArm': ((0,-0.05,-0.22),(0,-0.055,-0.30),'LeftArm'), 'LeftHand': ((0,-0.055,-0.30),(0,-0.055,-0.39),'LeftForeArm'),
 'RightArm': ((0,-0.05,0.11),(0,-0.05,0.22),'Spine2'), 'RightForeArm': ((0,-0.05,0.22),(0,-0.055,0.30),'RightArm'), 'RightHand': ((0,-0.055,0.30),(0,-0.055,0.39),'RightForeArm'),
 'LeftUpLeg': ((0,-0.26,-0.10),(0,-0.37,-0.10),'Hips'), 'LeftLeg': ((0,-0.37,-0.10),(0,-0.46,-0.10),'LeftUpLeg'), 'LeftFoot': ((0,-0.46,-0.10),(0.06,-0.485,-0.10),'LeftLeg'),
 'RightUpLeg': ((0,-0.26,0.10),(0,-0.37,0.10),'Hips'), 'RightLeg': ((0,-0.37,0.10),(0,-0.46,0.10),'RightUpLeg'), 'RightFoot': ((0,-0.46,0.10),(0.06,-0.485,0.10),'RightLeg'),
}
G2B = Matrix(((1,0,0),(0,0,-1),(0,1,0)))  # glTF Y-up -> Blender Z-up (same as importer)
def g2(p):
    v = G2B @ Vector(p); v.x *= front_sign if False else 1
    return XF @ v
arm_d = bpy.data.armatures.new('AngelRig'); arm = bpy.data.objects.new('AngelRig', arm_d)
bpy.context.scene.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm; bpy.ops.object.mode_set(mode='EDIT')
for n,(h,t,p) in G.items():
    bb = arm_d.edit_bones.new(n); bb.head = g2(h); bb.tail = g2(t)
for n,(h,t,p) in G.items():
    if p: arm_d.edit_bones[n].parent = arm_d.edit_bones[p]
hb = arm_d.edit_bones['Head']
sock = arm_d.edit_bones.new('head_hat'); sock.parent = hb; sock.use_deform = False
top = max((v.co for v in body.data.vertices), key=lambda c: c.z)
sock.head = Vector((hb.head.x, hb.head.y, top.z)); sock.tail = sock.head + Vector((0, 0, 0.05))
bpy.ops.object.mode_set(mode='OBJECT')
print('head bone z', round(arm_d.bones['Head'].head_local.z, 3), 'socket z', round(arm_d.bones['head_hat'].head_local.z, 3),
      'L hand', [round(c,3) for c in arm_d.bones['LeftHand'].head_local])
LEGS=['LeftUpLeg','LeftLeg','LeftFoot','RightUpLeg','RightLeg','RightFoot']; ARMS=['LeftArm','LeftForeArm','LeftHand','RightArm','RightForeArm','RightHand']
ALLOW = {'tripo_mesh_0':['Head','Neck'],'tripo_mesh_11':['Head'],'tripo_mesh_13':['Head'],
 'tripo_mesh_1':['Hips','Spine','Spine1','Spine2','LeftUpLeg','RightUpLeg'],'tripo_mesh_8':['Hips','Spine','Spine1','Spine2'],'tripo_mesh_12':['Hips','Spine','Spine1','Spine2','LeftUpLeg','RightUpLeg'],
 'tripo_mesh_7':['Spine1','Spine2','LeftArm','RightArm'],'tripo_mesh_4':LEGS,'tripo_mesh_5':LEGS,'tripo_mesh_9':ARMS+['Spine2'],'tripo_mesh_10':ARMS+['Spine2']}
from mathutils.geometry import intersect_point_line
def segd(c, bn):
    bo = arm_d.bones[bn]; a_, b_ = bo.head_local, bo.tail_local
    q, t = intersect_point_line(c, a_, b_); t = min(1, max(0, t)); return (c - (a_ + (b_-a_)*t)).length
vg = {n: body.vertex_groups.new(name=n) for n in G}
for v in body.data.vertices:
    allowed = ALLOW.get(part_of[v.index], list(G))
    d = sorted((segd(v.co, n), n) for n in allowed)
    w0 = 1/(d[0][0]+1e-3); w1 = 1/(d[1][0]+1e-3) if len(d) > 1 else 0
    if len(d) > 1 and d[1][0] < 1.6*d[0][0]:
        vg[d[0][1]].add([v.index], w0/(w0+w1), 'REPLACE'); vg[d[1][1]].add([v.index], w1/(w0+w1), 'REPLACE')
    else: vg[d[0][1]].add([v.index], 1.0, 'REPLACE')
body.parent = arm; md = body.modifiers.new('Armature', 'ARMATURE'); md.object = arm
print('weighted verts', sum(1 for v in body.data.vertices if len(v.groups)), '/', len(body.data.vertices))
# wings: rigid on Spine2
for w in wings:
    w.parent = arm; w.parent_type = 'BONE'; w.parent_bone = 'Spine2'
    mw = w.matrix_world.copy(); w.matrix_parent_inverse = (arm.matrix_world @ Matrix.Translation(arm.data.bones['Spine2'].tail_local)).inverted()
    w.matrix_world = mw
bpy.context.view_layer.update()

# --- report ---
def tris(o): return sum(len(p.vertices)-2 for p in o.data.polygons)
tb = tris(body); tw = sum(tris(w) for w in wings)
fuv = [body.data.uv_layers.active.data[li].uv for p in body.data.polygons if p.material_index == 2 for li in p.loop_indices]
print('MATERIAL SLOTS', len(body.material_slots), [s.material.name for s in body.material_slots])
print('TRIS body', tb, 'wings', tw, 'total', tb+tw)
print('FACE UV BBOX', round(min(u.x for u in fuv), 3), round(min(u.y for u in fuv), 3), round(max(u.x for u in fuv), 3), round(max(u.y for u in fuv), 3))
b0, b1 = wbb(body); print('DIMS m', [round(b1[i]-b0[i], 3) for i in range(3)])

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'angel.blend'), compress=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, 'angel.fbx'), use_selection=True, apply_scale_options='FBX_SCALE_UNITS',
                         object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE', primary_bone_axis='Y', secondary_bone_axis='X')
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, 'angel.glb'), export_format='GLB')
print('DONE')
