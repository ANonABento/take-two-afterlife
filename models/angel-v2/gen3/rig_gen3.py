# Rig gen 3 angel: 20 bones (17 body + head_hat + Wing.L/R). Input: out/angel_gen3_clean.ply (59,972 tris, 0 open edges, face -X, wings +X)
import bpy,bmesh,math
from mathutils import Vector
W="/tmp/lp4work"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_mesh.ply(filepath=W+"/out/angel_gen3_clean.ply")
ob=bpy.context.selected_objects[0]; bpy.context.view_layer.objects.active=ob
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.mesh.separate(type='LOOSE'); bpy.ops.object.mode_set(mode='OBJECT')
parts=sorted(bpy.context.selected_objects,key=lambda o:len(o.data.polygons),reverse=True)
body=parts[0]; wings=parts[1:3]
V=[body.matrix_world@v.co for v in body.data.vertices]
ys=[v.y for v in V]; ymax=max(ys); ymin=min(ys)
armz=sum(v.z for v in V if abs(v.y)>0.3)/max(1,sum(1 for v in V if abs(v.y)>0.3))
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
for w in wings:
  g=w.vertex_groups.new(name=w.name); g.add([v.index for v in w.data.vertices],1.0,'REPLACE')
bpy.ops.object.select_all(action='DESELECT')
for o in [body]+wings: o.select_set(True)
bpy.context.view_layer.objects.active=body; bpy.ops.object.join(); body.name="Angel"
for m in list(body.modifiers): body.modifiers.remove(m)
body.parent=ao; md=body.modifiers.new("Armature",'ARMATURE'); md.object=ao
bpy.ops.object.shade_smooth()
ao.select_set(True)
bpy.ops.export_scene.gltf(filepath=W+"/out/angel_gen3_rig.glb",export_format='GLB',use_selection=True)
# Arm pose sign: test both directions per arm, keep the one that lowers the hand (L=+1, R=-1).
