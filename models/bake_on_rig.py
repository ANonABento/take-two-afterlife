
import bpy, bmesh, math
from mathutils import Vector
from mathutils.bvhtree import BVHTree
OUT="/tmp/lp4work"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath="/tmp/lp4work/angel_gen3_rig.glb")
rig=[o for o in bpy.context.scene.objects if o.type=='MESH'][0]
arm=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
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
s=(rmx.z-rmn.z)/(amx.z-amn.z)

import numpy as np
from mathutils import Matrix
raw.scale=(s,s,s); bpy.context.view_layer.update()
def place(ang):
    raw.rotation_mode='XYZ'; raw.rotation_euler=(0,0,ang); raw.location=(0,0,0); bpy.context.view_layer.update()
    amn,amx=bb(raw); raw.location=Vector(((rmn.x+rmx.x)/2-(amn.x+amx.x)/2,(rmn.y+rmx.y)/2-(amn.y+amx.y)/2,rmn.z-amn.z)); bpy.context.view_layer.update()
def wingx():
    vs=[raw.matrix_world@v.co for v in list(raw.data.vertices)[::20]]
    b=[v.x for v in vs if 0.40<v.z<0.62 and abs(v.y)>0.12]
    return sum(b)/max(len(b),1)
scores=[]
for ang in (0,math.pi):
    place(ang); scores.append((wingx(),ang)); print("ANG",ang,"wing meanx",round(scores[-1][0],4))
res=[]
rv=None
for ang in (0,math.pi):
    place(ang)
    rv=np.array([tuple(rig.matrix_world@v.co) for v in list(rig.data.vertices)[::13]])
    def gapcalc():
        dg=bpy.context.evaluated_depsgraph_get(); tr=BVHTree.FromObject(raw,dg); mi=raw.matrix_world.inverted(); M=raw.matrix_world.copy()
        q=np.array([tuple(M@tr.find_nearest(mi@Vector(p))[0]) for p in rv]); return q,np.linalg.norm(q-rv,axis=1)
    for it in range(20):
        q,d=gapcalc(); keep=d<np.percentile(d,90)
        A=q[keep]; B=rv[keep]; ca=A.mean(0); cb=B.mean(0)
        U,S,Vt=np.linalg.svd((A-ca).T@(B-cb)); R=Vt.T@U.T
        if np.linalg.det(R)<0: Vt[-1]*=-1; R=Vt.T@U.T
        T=np.eye(4); T[:3,:3]=R; T[:3,3]=cb-R@ca
        raw.matrix_world=Matrix(T.tolist())@raw.matrix_world; bpy.context.view_layer.update()
        if it%5==0: print("ICP",it,round(float(d.mean()),4))
    q,d=gapcalc(); res.append((float(np.median(d)),ang,raw.matrix_world.copy())); print("ORIENT",ang,"median",round(res[-1][0],4))
res.sort(key=lambda x:x[0]); raw.matrix_world=res[0][2]; bpy.context.view_layer.update(); ang=res[0][1]; print("CHOSEN",ang)


q,d=gapcalc(); best=(float(np.median(d)),ang,None)
print("mean",round(float(d.mean()),4),"median",round(float(np.median(d)),4))
print("ALIGN mean gap m",round(best[0],4))
import sys
if best[0]>0.01: print("STOP gap too big"); sys.exit(0)
# slots
me=rig.data; me.materials.clear()
imgB=bpy.data.images.new("angel_basecolor",2048,2048); imgF=bpy.data.images.new("angel_face",1024,1024)
def mk(n,img):
    m=bpy.data.materials.new(n); m.use_nodes=True; nt=m.node_tree
    b=nt.nodes["Principled BSDF"]; b.inputs["Metallic"].default_value=0; b.inputs["Roughness"].default_value=0.8
    tn=nt.nodes.new("ShaderNodeTexImage"); tn.image=img; nt.links.new(tn.outputs[0],b.inputs[0]); nt.nodes.active=tn
    me.materials.append(m)
mk("Body",imgB); mk("Outfit",imgB); mk("Face",imgF)
bm=bmesh.new(); bm.from_mesh(me)
cx=(rmn.x+rmx.x)/2
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
ys=[(rig.matrix_world@me.vertices[i].co) for p in fl for i in p.vertices]
y0,y1=min(v.y for v in ys),max(v.y for v in ys); z0,z1=min(v.z for v in ys),max(v.z for v in ys)
for p in fl:
    for li in p.loop_indices:
        v=rig.matrix_world@me.vertices[me.loops[li].vertex_index].co
        uv[li].uv=((y1-v.y)/(y1-y0),(v.z-z0)/(z1-z0))
print("SLOTS",[sum(1 for p in me.polygons if p.material_index==i) for i in range(3)],"UVmaps",len(me.uv_layers))
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.device='CPU'; sc.cycles.samples=1
try: sc.cycles.use_denoising=False
except: pass
bk=sc.render.bake; bk.use_selected_to_active=True; bk.cage_extrusion=0.02; bk.margin=16
bk.use_pass_direct=False; bk.use_pass_indirect=False; bk.use_pass_color=True
bpy.ops.object.select_all(action='DESELECT'); raw.select_set(True); rig.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.object.bake(type='DIFFUSE')
for img,n in ((imgB,"angel_basecolor"),(imgF,"angel_face")):
    px=img.pixels[:]; tot=len(px)//4; fill=sum(1 for i in range(0,len(px),4*7) if px[i]+px[i+1]+px[i+2]>0.03)/(tot/7)
    print("FILL",n,round(fill,3))
    img.filepath_raw=OUT+"/"+n+".png"; img.file_format='PNG'; img.save()
bpy.data.objects.remove(raw)

sc.render.engine='BLENDER_WORKBENCH'; sh=sc.display.shading; sh.light='STUDIO'; sh.color_type='TEXTURE'
sh.background_type='WORLD'; sc.world=bpy.data.worlds.new('w'); sc.world.color=(1,1,1); sc.view_settings.view_transform='Standard'
sc.render.resolution_x=600; sc.render.resolution_y=800; sc.render.film_transparent=False
cam=bpy.data.objects.new("c",bpy.data.cameras.new("c")); sc.collection.objects.link(cam); sc.camera=cam
cam.data.type='ORTHO'; cam.data.ortho_scale=1.25
for nm,ang in (("front",math.pi),("side",-math.pi/2),("back",0)):
    d=Vector((math.cos(ang),math.sin(ang),0))*3
    cam.location=Vector((0,0,0.5))+d; cam.rotation_euler=(-d).to_track_quat('-Z','Y').to_euler()
    sc.render.filepath=OUT+"/v_"+nm+".png"; bpy.ops.render.render(write_still=True)
print("DONE")
