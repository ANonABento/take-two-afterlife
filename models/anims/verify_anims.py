# blender -b models/angel.blend --python verify_anims.py -- <fbx dir> : re-import each FBX, check bones/frames/in-place
import bpy, sys, os
OUT=os.path.abspath(sys.argv[sys.argv.index("--")+1])
ref=[b.name for b in bpy.data.objects["Armature"].data.bones]
EXP=dict(Idle=60,Run=18,JumpStart=12,FallLoop=15,Land=12,Hit=24,Cheer=60,Sit=60)
for n,N in EXP.items():
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(OUT,f"A_Angel_{n}.fbx"))
    new=[o for o in bpy.data.objects if o not in before]
    arm=[o for o in new if o.type=='ARMATURE'][0]; meshes=[o for o in new if o.type=='MESH']
    fr=tuple(round(x) for x in arm.animation_data.action.frame_range)
    md=0
    for f in range(fr[0],fr[1]+1):
        bpy.context.scene.frame_set(f); h=arm.matrix_world@arm.pose.bones["pelvis"].head; md=max(md,abs(h.x),abs(h.y))
    print("VERIFY",n,"range",fr,"len",fr[1]-fr[0],"exp",N,"OK" if fr[1]-fr[0]==N else "BAD","bones",sorted(b.name for b in arm.data.bones)==sorted(ref),
          "meshes",len(meshes),"objloc",tuple(arm.location),"pelvisXYmax",round(md,5))
    for o in new: bpy.data.objects.remove(o)
