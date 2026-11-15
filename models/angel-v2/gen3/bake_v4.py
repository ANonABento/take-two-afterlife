import os,sys,bpy,bmesh,math
import numpy as np
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
W=sys.argv[sys.argv.index("--")+1]; OUT=W+"/F"; os.makedirs(OUT,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=W+"/angel_gen3_rig.glb")
rig=[o for o in bpy.context.scene.objects if o.type=='MESH'][0]
before=set(bpy.context.scene.objects)
bpy.ops.import_scene.gltf(filepath=W+"/raw.glb")
raws=[o for o in bpy.context.scene.objects if o not in before and o.type=='MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in raws: o.select_set(True)
bpy.context.view_layer.objects.active=raws[0]
if len(raws)>1: bpy.ops.object.join()
raw=bpy.context.view_layer.objects.active
bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM'); bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
def bb(o):
    vs=np.array([tuple(o.matrix_world@v.co) for v in o.data.vertices]); return Vector(vs.min(0)),Vector(vs.max(0))
rmn,rmx=bb(rig); amn,amx=bb(raw); s=(rmx.z-rmn.z)/(amx.z-amn.z)
raw.scale=(s,s,s); raw.rotation_mode='XYZ'; raw.rotation_euler=(0,0,math.pi); bpy.context.view_layer.update()
amn,amx=bb(raw); raw.location=Vector(((rmn.x+rmx.x)/2-(amn.x+amx.x)/2,(rmn.y+rmx.y)/2-(amn.y+amx.y)/2,rmn.z-amn.z)); bpy.context.view_layer.update()
rv=np.array([tuple(rig.matrix_world@v.co) for v in list(rig.data.vertices)[::13]])
def gap():
    dg=bpy.context.evaluated_depsgraph_get(); tr=BVHTree.FromObject(raw,dg); mi=raw.matrix_world.inverted(); M=raw.matrix_world.copy()
    q=np.array([tuple(M@tr.find_nearest(mi@Vector(p))[0]) for p in rv]); return q,np.linalg.norm(q-rv,axis=1)
for it in range(12):
    q,d=gap(); k=d<np.percentile(d,90); A=q[k];B=rv[k];ca=A.mean(0);cb=B.mean(0)
    U,S,Vt=np.linalg.svd((A-ca).T@(B-cb)); R=Vt.T@U.T
    if np.linalg.det(R)<0: Vt[-1]*=-1; R=Vt.T@U.T
    T=np.eye(4);T[:3,:3]=R;T[:3,3]=cb-R@ca; raw.matrix_world=Matrix(T.tolist())@raw.matrix_world; bpy.context.view_layer.update()
q,d=gap(); print("GAP median",round(float(np.median(d)),5))
if np.median(d)>0.01: print("STOP"); sys.exit(0)
me=rig.data; me.materials.clear()
N=2048; img=bpy.data.images.new("angel_basecolor",N,N); imgF=bpy.data.images.new("angel_face",1024,1024)
def mk(n,im):
    m=bpy.data.materials.new(n); m.use_nodes=True; nt=m.node_tree; b=nt.nodes["Principled BSDF"]
    b.inputs["Metallic"].default_value=0; b.inputs["Roughness"].default_value=0.85
    tn=nt.nodes.new("ShaderNodeTexImage"); tn.image=im; nt.links.new(tn.outputs[0],b.inputs[0]); nt.nodes.active=tn; me.materials.append(m)
mk("Body",img); mk("Outfit",img); mk("Face",imgF)
cx=(rmn.x+rmx.x)/2
for p in me.polygons:
    c=rig.matrix_world@p.center; p.material_index=2 if (c.z>0.62 and c.x<cx-0.02 and p.normal.x<0) else 0
while me.uv_layers: me.uv_layers.remove(me.uv_layers[0])
me.uv_layers.new(name="UVMap")
bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='DESELECT')
rig.active_material_index=2; bpy.ops.object.material_slot_select(); bpy.ops.mesh.select_all(action='INVERT')
bpy.ops.uv.smart_project(angle_limit=math.radians(80),island_margin=0.01)
bpy.ops.object.mode_set(mode='OBJECT')
uv=me.uv_layers.active.data; fl=[p for p in me.polygons if p.material_index==2]
ys=[(rig.matrix_world@me.vertices[i].co) for p in fl for i in p.vertices]
y0,y1=min(v.y for v in ys),max(v.y for v in ys); z0,z1=min(v.z for v in ys),max(v.z for v in ys)
for p in fl:
    for li in p.loop_indices:
        v=rig.matrix_world@me.vertices[me.loops[li].vertex_index].co; uv[li].uv=((y1-v.y)/(y1-y0),(v.z-z0)/(z1-z0))
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.samples=1
try: sc.cycles.use_denoising=False
except: pass
bk=sc.render.bake; bk.use_selected_to_active=True; bk.cage_extrusion=0.02; bk.margin=4
bk.use_pass_direct=False; bk.use_pass_indirect=False; bk.use_pass_color=True
bpy.ops.object.select_all(action='DESELECT'); raw.select_set(True); rig.select_set(True); bpy.context.view_layer.objects.active=rig
bpy.ops.object.bake(type='DIFFUSE')
bpy.data.objects.remove(raw)
src=np.array(img.pixels[:]).reshape(N,N,4)[:,:,:3]
# per-face colour decision
P=len(me.polygons); fuv=np.zeros((P,2)); isblue=np.zeros(P,bool)
for p in me.polygons:
    u=np.array([uv[li].uv[:] for li in p.loop_indices]).mean(0); fuv[p.index]=u
    if p.material_index!=2:
        x=int(np.clip(u[0]*N,0,N-1)); y=int(np.clip(u[1]*N,0,N-1)); c=src[y,x]
        isblue[p.index]=(c[2]-c[0])>0.04
bm=bmesh.new(); bm.from_mesh(me); bm.faces.ensure_lookup_table()
nb=[[g.index for e in f.edges for g in e.link_faces if g.index!=f.index] for f in bm.faces]; bm.free()
face=np.array([p.material_index==2 for p in me.polygons])
for it in range(6):
    nw=isblue.copy()
    for i in range(P):
        if face[i] or not nb[i]: continue
        v=isblue[nb[i]].mean(); nw[i]=v>0.5 if v!=0.5 else isblue[i]
    isblue=nw
for p in me.polygons:
    if p.material_index!=2: p.material_index=1 if isblue[p.index] else 0
print("SLOTS",[int((np.array([p.material_index for p in me.polygons])==i).sum()) for i in range(3)])
blue=np.array([0xA9,0xC6,0xE3])/255.0; white=np.array([0xF2,0xF0,0xEC])/255.0
def lin(c): return np.where(c<=0.04045,c/12.92,((c+0.055)/1.055)**2.4)
L=src.mean(2); valid=L>0.01; med=np.median(L[valid])
shade=np.clip(L/med,0.85,1.03)  # speck clamp
out=np.zeros((N,N,3)); filled=np.zeros((N,N),bool)
for p in me.polygons:
    if p.material_index==2: continue
    col=lin(blue) if p.material_index==1 else lin(white)
    t=np.array([uv[li].uv[:] for li in p.loop_indices])*N
    for k in range(1,len(t)-1):
        a,b,c=t[0],t[k],t[k+1]
        x0,x1=int(max(min(a[0],b[0],c[0])-1,0)),int(min(max(a[0],b[0],c[0])+1,N-1))
        y0_,y1_=int(max(min(a[1],b[1],c[1])-1,0)),int(min(max(a[1],b[1],c[1])+1,N-1))
        X,Y=np.meshgrid(np.arange(x0,x1+1)+0.5,np.arange(y0_,y1_+1)+0.5)
        den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(den)<1e-9: continue
        l1=((b[1]-c[1])*(X-c[0])+(c[0]-b[0])*(Y-c[1]))/den; l2=((c[1]-a[1])*(X-c[0])+(a[0]-c[0])*(Y-c[1]))/den
        m=(l1>=-0.02)&(l2>=-0.02)&(1-l1-l2>=-0.02)
        yy,xx=(Y[m]-0.5).astype(int),(X[m]-0.5).astype(int)
        out[yy,xx]=col*shade[yy,xx,None]; filled[yy,xx]=True
for it in range(32):
    acc=np.zeros_like(out); cnt=np.zeros((N,N))
    for dy,dx in ((1,0),(-1,0),(0,1),(0,-1)):
        acc+=np.roll(np.roll(out*filled[...,None],dy,0),dx,1); cnt+=np.roll(np.roll(filled,dy,0),dx,1)
    new=(~filled)&(cnt>0); out[new]=acc[new]/cnt[new,None]; filled|=new
a=np.ones((N,N,4)); a[:,:,:3]=np.clip(out,0,1); img.pixels=a.ravel().tolist()
f=np.ones((1024*1024,4)); f[:,:3]=lin(white); imgF.pixels=f.ravel().tolist()
for im in (img,imgF): im.pack()
sc.render.engine='BLENDER_WORKBENCH'; sh=sc.display.shading; sh.light='STUDIO'; sh.color_type='TEXTURE'
sh.background_type='VIEWPORT'; sh.background_color=(0,0,0); sc.view_settings.view_transform='Standard'
sc.render.resolution_x=600; sc.render.resolution_y=800
cam=bpy.data.objects.new("c",bpy.data.cameras.new("c")); sc.collection.objects.link(cam); sc.camera=cam
cam.data.type='ORTHO'; cam.data.ortho_scale=1.25
for nm,ang in (("front",math.pi),("side",-math.pi/2),("back",0)):
    dd=Vector((math.cos(ang),math.sin(ang),0))*3; cam.location=Vector((0,0,0.5))+dd; cam.rotation_euler=(-dd).to_track_quat('-Z','Y').to_euler()
    sc.render.filepath=OUT+"/render_"+nm+".png"; bpy.ops.render.render(write_still=True)
bpy.data.objects.remove(cam)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=OUT+"/angel.glb",export_format='GLB')
bpy.ops.export_scene.fbx(filepath=OUT+"/SK_Angel.fbx",path_mode='COPY',embed_textures=True,add_leaf_bones=False)
print("EXPORTED",[(x,os.path.getsize(OUT+"/"+x)) for x in os.listdir(OUT)])
