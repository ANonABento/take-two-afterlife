# Rig the cleaned gen 3 angel. Input: <dir>/out/angel_gen3_clean.ply (from cleanup_gen3.py).
# Run: blender -b --python rig_gen3.py -- <dir>
import bpy,bmesh,math,sys
from mathutils import Vector
W=sys.argv[sys.argv.index("--")+1] if "--" in sys.argv else "."
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_mesh.ply(filepath=W+"/out/angel_gen3_clean.ply")
ob=bpy.context.selected_objects[0]; bpy.context.view_layer.objects.active=ob
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.separate(type='LOOSE'); bpy.ops.object.mode_set(mode='OBJECT')
parts=sorted(bpy.context.selected_objects,key=lambda o:len(o.data.polygons),reverse=True)
body=parts[0]; wings=parts[1:3]
print("PARTS",[len(p.data.polygons) for p in parts])
V=[body.matrix_world@v.co for v in body.data.vertices]
ys=[v.y for v in V]; ymax=max(ys); ymin=min(ys)
armz=sum(v.z for v in V if abs(v.y)>0.3)/max(1,sum(1 for v in V if abs(v.y)>0.3))
print("ARMZ",armz,"Y",ymin,ymax)
arm=bpy.data.armatures.new("Rig"); ao=bpy.data.objects.new("Rig",arm); bpy.context.collection.objects.link(ao)
bpy.context.view_layer.objects.active=ao; bpy.ops.object.mode_set(mode='EDIT')
def b(n,h,t,p=None):
  e=arm.edit_bones.new(n); e.head=h; e.tail=t
  if p: e.parent=arm.edit_bones[p]; e.use_connect=False
sz=armz
b("Hips",(0,0,0.20),(0,0,0.30)); b("Spine",(0,0,0.30),(0,0,0.38),"Hips"); b("Spine2",(0,0,0.38),(0,0,sz),"Spine")
b("Neck",(0,0,sz),(0,0,sz+0.06),"Spine2"); b("Head",(0,0,sz+0.06),(0,0,0.95),"Neck"); b("head_hat",(0,0,1.0),(0,0,1.08),"Head")
for S,k in (("L",1),("R",-1)):
  ye=ymax if k>0 else ymin
  b("Shoulder."+S,(0,k*0.04,sz),(0,k*0.10,sz),"Spine2"); b("UpperArm."+S,(0,k*0.10,sz),(0,k*0.22,sz),"Shoulder."+S)
  b("ForeArm."+S,(0,k*0.22,sz),(0,k*0.34,sz),"UpperArm."+S); b("Hand."+S,(0,k*0.34,sz),(0,ye,sz),"ForeArm."+S)
  b("UpLeg."+S,(0,k*0.07,0.20),(0,k*0.07,0.10),"Hips"); b("Leg."+S,(0,k*0.07,0.10),(0,k*0.07,0.0),"UpLeg."+S)
for w in wings:
  c=sum((w.matrix_world@v.co for v in w.data.vertices),Vector())/len(w.data.vertices)
  S="L" if c.y>0 else "R"; w.name="Wing."+S
  b("Wing."+S,(0.06,c.y*0.4,sz+0.02),(c.x,c.y,c.z),"Spine2")
bpy.ops.object.mode_set(mode='OBJECT')
print("BONES",len(arm.bones))
bpy.ops.object.select_all(action='DESELECT'); body.select_set(True); ao.select_set(True); bpy.context.view_layer.objects.active=ao
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
for g in list(body.vertex_groups):
  if g.name.startswith("Wing") or g.name=="head_hat": body.vertex_groups.remove(g)
import mathutils
kd=mathutils.kdtree.KDTree(len(body.data.vertices))
for v in body.data.vertices:
  if any(g.weight>0 for g in v.groups): kd.insert(v.co,v.index)
kd.balance()
for v in body.data.vertices:
  if not any(g.weight>0 for g in v.groups):
    co,idx,d=kd.find(v.co)
    for g in body.data.vertices[idx].groups: body.vertex_groups[g.group].add([v.index],g.weight,'REPLACE')
unw=sum(1 for v in body.data.vertices if not any(g.weight>0 for g in v.groups))
print("UNWEIGHTED",unw,"OF",len(body.data.vertices))
for w in wings:
  g=w.vertex_groups.new(name=w.name); g.add([v.index for v in w.data.vertices],1.0,'REPLACE')
bpy.ops.object.select_all(action='DESELECT')
for o in [body]+wings: o.select_set(True)
bpy.context.view_layer.objects.active=body; bpy.ops.object.join(); body.name="Angel"
for m in list(body.modifiers): body.modifiers.remove(m)
body.parent=ao; md=body.modifiers.new("Armature",'ARMATURE'); md.object=ao
bpy.ops.object.select_all(action='DESELECT'); body.select_set(True); bpy.context.view_layer.objects.active=body; bpy.ops.object.shade_smooth()
print("TRIS",len(body.data.polygons),"GROUPS",len(body.vertex_groups))
ao.select_set(True)
bpy.ops.export_scene.gltf(filepath=W+"/out/angel_gen3_rig.glb",export_format='GLB',use_selection=True)
# test pose
pb=ao.pose.bones
for S,k in (("L",1),("R",-1)):
  # fixed arm-down sign, verified on gen 3: L=+1, R=-1 (both hands end at z=0.248)
  sg={"L":1,"R":-1}[S]
  pb["UpperArm."+S].rotation_mode='XYZ'; pb["UpperArm."+S].rotation_euler.x=math.radians(-50*k*sg)
  pb["ForeArm."+S].rotation_mode='XYZ'; pb["ForeArm."+S].rotation_euler.z=math.radians(30)
  pb["UpLeg."+S].rotation_mode='XYZ'; pb["UpLeg."+S].rotation_euler.x=math.radians(-12*k)
  pb["Wing."+S].rotation_mode='XYZ'; pb["Wing."+S].rotation_euler.z=math.radians(25*k)
bpy.context.view_layer.update()
print("HANDZ",{S:round((ao.matrix_world@ao.pose.bones["Hand."+S].tail).z,3) for S in "LR"})
sc=bpy.context.scene; sc.render.engine='BLENDER_WORKBENCH'
sh=sc.display.shading; sh.light='STUDIO'; sh.color_type='SINGLE'; sh.single_color=(0.75,0.75,0.75)
sc.view_settings.view_transform='Standard'
w=bpy.data.worlds.new('W'); w.color=(1,1,1); sc.world=w; sh.background_type='WORLD'; sh.show_cavity=False; sh.show_object_outline=False
sc.render.resolution_x=640; sc.render.resolution_y=800
cam=bpy.data.objects.new("Cam",bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera=cam
cam.data.type='ORTHO'; cam.data.ortho_scale=1.25
for i,(lab,ang) in enumerate((("front",180),("side",90),("back",0))):
  a=math.radians(ang); cam.location=(3*math.cos(a),3*math.sin(a),0.5)
  d=Vector((0,0,0.5))-cam.location; cam.rotation_euler=d.to_track_quat('-Z','Y').to_euler()
  sc.render.filepath=W+"/out/pose_%d_%s.png"%(i,lab); bpy.ops.render.render(write_still=True)
print("DONE")
