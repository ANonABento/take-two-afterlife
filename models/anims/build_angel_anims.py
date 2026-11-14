# Build + export SK_Angel animation set. blender -b models/angel.blend --python build_angel_anims.py -- <outdir>
import bpy, math, sys, os
OUT = os.path.abspath(sys.argv[sys.argv.index("--")+1]); os.makedirs(OUT, exist_ok=True)
sc = bpy.context.scene; sc.render.fps = 30; sc.render.fps_base = 1
A = bpy.data.objects["Armature"]
for o in bpy.data.objects:
    o.select_set(False)
A.location = (0,0,0); A.rotation_euler = (0,0,0)
if A.animation_data is None: A.animation_data_create()
for pb in A.pose.bones: pb.rotation_mode = 'XYZ'
S = math.sin; TAU = 2*math.pi; D = math.radians

def make(name, N, pose_fn, step=1):
    act = bpy.data.actions.new("A_Angel_"+name); act.use_fake_user = True
    A.animation_data.action = act
    frames = sorted(set(list(range(0, N+1, step)) + [N]))
    for f in frames:
        t = f / N  # 0..1, loops: t=0 and t=1 identical
        for pb in A.pose.bones:
            pb.location = (0,0,0); pb.rotation_euler = (0,0,0)
        for b, (rot, loc) in pose_fn(t).items():
            pb = A.pose.bones[b]
            pb.rotation_euler = [D(r) for r in rot]
            if loc: pb.location = (0, loc, 0)   # pelvis local Y = world up only (in place)
        for pb in A.pose.bones:
            pb.keyframe_insert("rotation_euler", frame=f)
            pb.keyframe_insert("location", frame=f)
    act.frame_range = (0, N)
    return act

def P(**k):  # helper: bone=(rx,ry,rz) or ((rx,ry,rz), up)
    d = {}
    for b, v in k.items():
        d[b] = (v[0], v[1]) if isinstance(v[0], tuple) else (v, None)
    return d

def arms(rx, rz, lx=0, lz=0, rxr=None, rzr=None):
    rxr = rx if rxr is None else rxr; rzr = rz if rzr is None else rzr
    return dict(upperarm_l=(rx,0,rz), upperarm_r=(rxr,0,-rzr), lowerarm_l=(lx,0,lz), lowerarm_r=(lx,0,-lz))

def idle(t):
    w = S(TAU*t); w2 = S(2*TAU*t)  # 1 float cycle, 2 flaps
    d = P(pelvis=((0,0,0), 0.03+0.025*w), spine=(2*w2,0,0), head=(-3*w,0,4*S(TAU*t+1)),
          thigh_l=(-8,0,0), calf_l=(16,0,0), thigh_r=(-4,0,0), calf_r=(10,0,0), foot_l=(-10,0,0), foot_r=(-8,0,0))
    a = arms(0, 25+12*w2, 0, 10+8*w2)   # arms flap like little wings
    d.update({k:(v,None) for k,v in a.items()}); return d

def run(t):
    c = S(TAU*t); b = abs(S(TAU*t))
    d = P(pelvis=((0,0,6*c), 0.035*b), spine=(-12,0,-8*c), head=(8,0,6*c),
          thigh_l=(-45*c,0,0), thigh_r=(45*c,0,0),
          calf_l=(30+30*max(0,c),0,0), calf_r=(30+30*max(0,-c),0,0),
          foot_l=(-15*c,0,0), foot_r=(15*c,0,0))
    d.update({k:(v,None) for k,v in dict(upperarm_l=(50*c,0,20), upperarm_r=(-50*c,0,-20),
              lowerarm_l=(-50,0,0), lowerarm_r=(-50,0,0)).items()}); return d

def crouch(k, up=0.0):  # k 0..1 crouch amount
    return P(pelvis=((0,0,0), -0.08*k+up), spine=(10*k,0,0), head=(-8*k,0,0),
             thigh_l=(-50*k,0,0), thigh_r=(-50*k,0,0), calf_l=(90*k,0,0), calf_r=(90*k,0,0),
             foot_l=(-40*k,0,0), foot_r=(-40*k,0,0))

def jump(t):  # 12 f: squash then stretch/launch
    if t < 0.5: k = math.sin(math.pi*t); d = crouch(k*1.0); a = arms(-30*k, 15)
    else:
        s = (t-0.5)/0.5; d = crouch(max(0,1-2*s)*0.5, 0.06*s)
        d["spine"] = ((-8*s,0,0), None); d["thigh_l"]=((-20*s,0,0),None); d["calf_l"]=((40*s,0,0),None)
        a = arms(-30+(-120+30)*s, 15+10*s)   # arms whip up
    d.update({k:(v,None) for k,v in a.items()}); return d

def fall(t):
    w = S(TAU*t)
    d = P(pelvis=((0,0,0), 0.04), spine=(-6+3*w,0,0), head=(-12,0,0),
          thigh_l=(-25+10*w,0,0), calf_l=(45,0,0), thigh_r=(-10-10*w,0,0), calf_r=(30,0,0),
          foot_l=(10,0,0), foot_r=(10,0,0))
    d.update({k:(v,None) for k,v in arms(-20, 70+15*w, 0, 20+15*w).items()}); return d  # flailing

def land(t):
    k = math.sin(math.pi*min(1, t*1.6)) if t < 0.62 else 0
    k = max(k, 0) * (1 if t < 0.62 else 0)
    d = crouch(k); d.update({kk:(v,None) for kk,v in arms(-20*k, 25+40*k, 0, 10).items()})
    return d

def hit(t):  # 24 f stumble back and recover
    k = math.sin(math.pi*t)**0.7; w = S(3*TAU*t)*(1-t)
    d = P(pelvis=((-10*k,0,8*w), -0.04*k), spine=(-25*k,0,10*w), head=(-20*k,0,15*w),
          thigh_l=(25*k,0,0), calf_l=(20*k,0,0), thigh_r=(-30*k,0,0), calf_r=(40*k,0,0),
          foot_l=(-10*k,0,0), foot_r=(-10*k,0,0))
    d.update({kk:(v,None) for kk,v in arms(-40*k, 30+50*k, -20*k, 30*k).items()}); return d

def cheer(t):  # 2 s: two hops, arms pumping overhead
    h = abs(S(2*TAU*t)); w = S(4*TAU*t)
    d = crouch(0.25*(1-h), 0.07*h)
    d["spine"] = ((-8,0,6*S(TAU*t)), None); d["head"] = ((-15,0,10*S(TAU*t)), None)
    d.update({kk:(v,None) for kk,v in arms(-150+20*w, 20, -10, 15+20*w).items()}); return d

def sit(t):  # couch sit, gentle breathing / foot swing
    w = S(TAU*t)
    d = P(pelvis=((0,0,0), -0.12+0.005*w), spine=(-4+2*w,0,0), head=(3*w,0,5*S(TAU*t+0.5)),
          thigh_l=(-90,0,0), thigh_r=(-90,0,0), calf_l=(90+10*w,0,0), calf_r=(90-10*w,0,0),
          foot_l=(-5,0,0), foot_r=(-5,0,0))
    d.update({kk:(v,None) for kk,v in arms(-30, 10, -40, 0).items()}); return d

SPEC = [("Idle",60,idle),("Run",18,run),("JumpStart",12,jump),("FallLoop",15,fall),
        ("Land",12,land),("Hit",24,hit),("Cheer",60,cheer),("Sit",60,sit)]
for n,N,f in SPEC:
    act = make(n, N, f)
    sc.frame_start, sc.frame_end = 0, N
    bpy.ops.object.select_all(action='DESELECT'); A.select_set(True); bpy.context.view_layer.objects.active = A
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, f"A_Angel_{n}.fbx"), use_selection=True,
        object_types={'ARMATURE'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
        apply_unit_scale=True, axis_forward='-Y', axis_up='Z')
    print("EXPORTED", n, N)
A.animation_data.action = None
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "angel_anims.blend"))
