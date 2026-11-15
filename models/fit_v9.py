# v9 band fit. blender -b angel.blend --python fit_v9.py -- <outdir>
# Face is at -X. Beanie/viking: rigid, head goes inside (overlap allowed).
# Ears/horns headbands: wrapped onto the skull (polar remap), ears/horns ride along.
import bpy, bmesh, numpy as np, sys, os, json, math
from mathutils import Vector
OUT = sys.argv[sys.argv.index("--")+1] if "--" in sys.argv else "."
os.makedirs(OUT, exist_ok=True)
dg = bpy.context.evaluated_depsgraph_get()
# ---- head profile from posed body (x-slice), polar about C ----
P=[]
for o in bpy.data.objects:
    if o.type=='MESH' and o.visible_get():
        e=o.evaluated_get(dg); m=e.to_mesh()
        a=np.empty(len(m.vertices)*3); m.vertices.foreach_get("co",a); a=a.reshape(-1,3)
        M=np.array(o.matrix_world); a=a@M[:3,:3].T+M[:3,3]; P.append(a); e.to_mesh_clear()
P=np.vstack(P)
C=np.array([0.005,0.75])
s=P[(np.abs(P[:,0])<0.03)&(P[:,2]>0.6)]
th=np.arctan2(s[:,2]-C[1],s[:,1]-C[0]); rr=np.hypot(s[:,1]-C[0],s[:,2]-C[1])
bins=np.linspace(-0.3,math.pi+0.3,50); mid=[];R=[]
for a,b in zip(bins[:-1],bins[1:]):
    k=(th>=a)&(th<b)
    if k.any(): mid.append((a+b)/2); R.append(rr[k].max())
mid=np.array(mid); R=np.array(R)
def skullR(t): return np.interp(t,mid,R)
names=["propeller-beanie","viking-helmet","bunny-ears","devil-horns"]
HATDIR=os.environ.get("HATDIR",".")
def imp(n):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(HATDIR,f"{n}.glb"))
    new=[o for o in bpy.data.objects if o not in before and o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in new: o.select_set(True)
    bpy.context.view_layer.objects.active=new[0]
    if len(new)>1: bpy.ops.object.join()
    o=bpy.context.view_layer.objects.active
    bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    for x in list(bpy.data.objects):
        if x not in before and x is not o: bpy.data.objects.remove(x)
    o.name=n; return o
def getv(o):
    a=np.empty(len(o.data.vertices)*3); o.data.vertices.foreach_get("co",a); return a.reshape(-1,3)
def setv(o,a): o.data.vertices.foreach_set("co",a.ravel()); o.data.update()
def rz(a,deg):
    c,s_=math.cos(math.radians(deg)),math.sin(math.radians(deg)); return a@np.array([[c,-s_,0],[s_,c,0],[0,0,1]]).T
def ry(a,deg):
    c,s_=math.cos(math.radians(deg)),math.sin(math.radians(deg)); return a@np.array([[c,0,s_],[0,1,0],[-s_,0,c]]).T
RIGID={"propeller-beanie":dict(rot=0,s=2.0,tilt=14,rim=0.80,dz=-0.10),
       "viking-helmet":dict(rot=180,s=2.2,tilt=0,rim=0.78,dz=-0.02)}
BAND={"bunny-ears":dict(s=2.1,zt=0.145),"devil-horns":dict(s=1.8,zt=0.245)}
fit={}
for n in names:
    o=imp(n); v=getv(o); b0,b1=v.min(0),v.max(0)
    v=v-[(b0[0]+b1[0])/2,(b0[1]+b1[1])/2,b0[2]]
    if n in RIGID:
        p=RIGID[n]; v=v*p["s"]; v=rz(v,p["rot"])
        if p["tilt"]: v=ry(v,p["tilt"])
        v=v+[0,0.005,p["rim"]+p["dz"]]
        fit[n]=dict(kind="band_rigid",scale=p["s"],rot_z_deg=p["rot"],tilt_y_deg=p["tilt"],
                    world_offset=[0,0.005,round(p["rim"]+p["dz"],4)],overlap_allowed=True)
    else:
        p=BAND[n]; v=v*p["s"]
        v=v[:,[1,0,2]]*[1,1,1]          # band width -> world Y, thin -> world X
        band=(np.abs(v[:,1])<0.06*p["s"])&(v[:,2]<p["zt"]*p["s"])
        zc=0.5*p["zt"]*p["s"]
        phi=np.arctan2(v[:,2]-zc,v[:,1]); phi=np.where(phi<-math.pi/2,phi+2*math.pi,phi)
        rho=np.hypot(v[:,1],v[:,2]-zc)
        pb=phi[band]; lo,hi=pb.min(),pb.max()
        # inner band radius per phi bin
        nb=40; eb=np.linspace(lo,hi,nb+1); cen=(eb[:-1]+eb[1:])/2; rin=[]
        for a,c in zip(eb[:-1],eb[1:]):
            k=band&(phi>=a)&(phi<=c); rin.append(rho[k].min() if k.any() else np.nan)
        rin=np.array(rin); ok=~np.isnan(rin); rin_at=np.interp(phi,cen[ok],rin[ok])
        T0,T1=0.30,math.pi-0.30
        t=T0+(np.clip(phi,lo,hi)-lo)/(hi-lo)*(T1-T0)
        rad=skullR(t)+0.003+np.maximum(rho-rin_at,0)
        nv=np.zeros_like(v); nv[:,0]=v[:,0]; nv[:,1]=C[0]+rad*np.cos(t); nv[:,2]=C[1]+rad*np.sin(t)
        v=nv
        fit[n]=dict(kind="band_wrap",scale=p["s"],rot_z_deg=90,wrap_theta_rad=[T0,T1],
                    clearance_m=0.003,note="mesh deformed to skull; vertices baked",overlap_allowed=True)
    setv(o,v)
    # parent to head_hat keeping world
    rig=next((x for x in bpy.data.objects if x.type=='ARMATURE'),None)
    if rig and "head_hat" in rig.data.bones:
        mw=o.matrix_world.copy(); o.parent=rig; o.parent_type='BONE'; o.parent_bone="head_hat"; o.matrix_world=mw
        fit[n]["parent"]="Rig/head_hat"; fit[n]["local_location"]=[round(x,4) for x in o.location]
    bpy.context.view_layer.update(); d=o.dimensions; fit[n]["size_m"]=[round(d.x,3),round(d.y,3),round(d.z,3)]
    # export fitted mesh for checks
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.update()
    _m=o.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh(); _m.calc_loop_triangles()
    _v=np.array([o.matrix_world@x.co for x in _m.vertices]); _f=np.array([t.vertices[:] for t in _m.loop_triangles])
    np.savez_compressed(os.path.join(OUT,f"{n}_fit.npz"),v=_v.astype(np.float32),f=_f.astype(np.int32))
json.dump(fit,open(os.path.join(OUT,"fit_v9.json"),"w"),indent=1)
# ---- render sheet ----
sc=bpy.context.scene; sc.render.engine='BLENDER_WORKBENCH'
sc.display.shading.light='STUDIO'; sc.display.shading.color_type='TEXTURE'
sc.render.resolution_x=sc.render.resolution_y=360; sc.render.image_settings.file_format='PNG'
cam_d=bpy.data.cameras.new("c"); cam_d.type='ORTHO'; cam_d.ortho_scale=1.0
cam=bpy.data.objects.new("c",cam_d); sc.collection.objects.link(cam); sc.camera=cam
tgt=Vector((0,0,0.88))
views={"front":Vector((-1,0,0)),"side":Vector((0,-1,0)),"q34":Vector((-0.7,-0.7,0.15)).normalized()}
for n in names:
    for m in names: bpy.data.objects[m].hide_render=(m!=n)
    for k,dv in views.items():
        cam.location=tgt+dv*3; cam.rotation_euler=(-dv).to_track_quat('-Z','Y').to_euler()
        sc.render.filepath=os.path.join(OUT,f"r_{n}_{k}.png"); bpy.ops.render.render(write_still=True)
print("V9DONE")
