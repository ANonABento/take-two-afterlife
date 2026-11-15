# Bake gen 3 raw Tripo colour onto the rig's own UVs. Raw: download from task bd71ffe3-52b3-427a-a6c0-9a6f139c8773 to /tmp/raw.glb.
# KEY FIX: glTF import uses quaternion rotation; set rotation_mode='XYZ' before turning, then ICP-fit. Fit reached 0.1 mm.
import bpy, bmesh, math, sys
import numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
OUT="/tmp/lp4work"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath="/tmp/lp4work/angel_gen3_rig.glb")
rig=[o for o in bpy.context.scene.objects if o.type=='MESH'][0]
before=set(bpy.context.scene.objects)
bpy.ops.import_scene.gltf(filepath="/tmp/raw.glb")
raws=[o for o in bpy.context.scene.objects if o not in before and o.type=='MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in raws: o.select_set(True)
bpy.context.view_layer.objects.active=raws[0]
if len(raws)>1: bpy.ops.object.join()
raw=bpy.context.view_layer.objects.active
bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM'); bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
def bb(o):
    vs=[o.matrix_world@v.co for v in o.data.vertices]
    return Vector([min(v[i] for v in vs) for i in range(3)]),Vector([max(v[i] for v in vs) for i in range(3)])
rmn,rmx=bb(rig); amn,amx=bb(raw)
s=(rmx.z-rmn.z)/(amx.z-amn.z); raw.scale=(s,s,s); bpy.context.view_layer.update()
def place(ang):
    raw.rotation_mode='XYZ'; raw.rotation_euler=(0,0,ang); raw.location=(0,0,0); bpy.context.view_layer.update()
    amn,amx=bb(raw); raw.location=Vector(((rmn.x+rmx.x)/2-(amn.x+amx.x)/2,(rmn.y+rmx.y)/2-(amn.y+amx.y)/2,rmn.z-amn.z)); bpy.context.view_layer.update()
rv=np.array([tuple(rig.matrix_world@v.co) for v in list(rig.data.vertices)[::13]])
def gapcalc():
    dg=bpy.context.evaluated_depsgraph_get(); tr=BVHTree.FromObject(raw,dg); mi=raw.matrix_world.inverted(); M=raw.matrix_world.copy()
    q=np.array([tuple(M@tr.find_nearest(mi@Vector(p))[0]) for p in rv]); return q,np.linalg.norm(q-rv,axis=1)
res=[]
for ang in (0,math.pi):
    place(ang)
    for it in range(20):
        q,d=gapcalc(); keep=d<np.percentile(d,90)
        A=q[keep]; B=rv[keep]; ca=A.mean(0); cb=B.mean(0)
        U,S,Vt=np.linalg.svd((A-ca).T@(B-cb)); R=Vt.T@U.T
        if np.linalg.det(R)<0: Vt[-1]*=-1; R=Vt.T@U.T
        T=np.eye(4); T[:3,:3]=R; T[:3,3]=cb-R@ca
        raw.matrix_world=Matrix(T.tolist())@raw.matrix_world; bpy.context.view_layer.update()
    q,d=gapcalc(); res.append((float(np.median(d)),ang,raw.matrix_world.copy()))
res.sort(key=lambda x:x[0]); raw.matrix_world=res[0][2]; bpy.context.view_layer.update()
q,d=gapcalc(); gap=float(np.median(d)); print("ALIGN median gap m",round(gap,4))
if gap>0.01: print("STOP gap too big"); sys.exit(0)
me=rig.data; me.materials.clear()
imgB=bpy.data.images.new("angel_basecolor",2048,2048); imgF=bpy.data.images.new("angel_face",1024,1024)
def mk(n,img):
    m=bpy.data.materials.new(n); m.use_nodes=True; nt=m.node_tree
    b=nt.nodes["Principled BSDF"]; b.inputs["Metallic"].default_value=0; b.inputs["Roughness"].default_value=0.8
    tn=nt.nodes.new("ShaderNodeTexImage"); tn.image=img; nt.links.new(tn.outputs[0],b.inputs[0]); nt.nodes.active=tn
    me.materials.append(m)
mk("Body",imgB); mk("Outfit",imgB); mk("Face",imgF)
bm=bmesh.new(); bm.from_mesh(me); cx=(rmn.x+rmx.x)/2
for f in bm.faces:
    c=rig.matrix_world@f.calc_center_median(); n=f.normal
    if c.z>0.62 and c.x<cx-0.02 and n.x<0: f.material_index=2
    elif 0.12<c.z<0.50: f.material_index=1
    else: f.material_index=0
bm.to_mesh(me); bm.free()
while me.uv_layers: me.uv_layers.remove(me.uv_layers[0])
me.uv_layers.new(name="UVMap")
bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='DESELECT')
rig.active_material_index=2; bpy.ops.object.material_slot_select(); bpy.ops.mesh.select_all(action='INVERT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=0.003)
bpy.ops.object.mode_set(mode='OBJECT')
uv=me.uv_layers.active.data; fl=[p for p in me.polygons if p.material_index==2]
vs=[(rig.matrix_world@me.vertices[i].co) for p in fl for i in p.vertices]
y0,y1=min(v.y for v in vs),max(v.y for v in vs); z0,z1=min(v.z for v in vs),max(v.z for v in vs)
for p in fl:
    for li in p.loop_indices:
        v=rig.matrix_world@me.vertices[me.loops[li].vertex_index].co
        uv[li].uv=((y1-v.y)/(y1-y0),(v.z-z0)/(z1-z0))
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.device='CPU'; sc.cycles.samples=1
try: sc.cycles.use_denoising=False
except: pass
bk=sc.render.bake; bk.use_selected_to_active=True; bk.cage_extrusion=0.02; bk.margin=16
bk.use_pass_direct=False; bk.use_pass_indirect=False; bk.use_pass_color=True
bpy.ops.object.select_all(action='DESELECT'); raw.select_set(True); rig.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.object.bake(type='DIFFUSE')
for img,n in ((imgB,"angel_basecolor"),(imgF,"angel_face")):
    img.filepath_raw=OUT+"/"+n+".png"; img.file_format='PNG'; img.save()
