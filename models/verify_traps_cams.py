import bpy, sys, os, glob
from mathutils import Vector
d = os.path.abspath(sys.argv[sys.argv.index('--')+1])
bad = 0
for f in sorted(glob.glob(os.path.join(d, '*.fbx'))):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=f)
    obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    o = obs[0]; dg = bpy.context.evaluated_depsgraph_get(); me = o.evaluated_get(dg).to_mesh(); me.calc_loop_triangles()
    vs = [o.matrix_world @ v.co for v in me.vertices]
    mn = [min(v[i] for v in vs)*100 for i in range(3)]; mx = [max(v[i] for v in vs)*100 for i in range(3)]
    piv = [x*100 for x in o.matrix_world.translation]; dims = [mx[i]-mn[i] for i in range(3)]
    tris = len(me.loop_triangles); name = os.path.basename(f)[:-4]
    ok = tris < 2500 and len(obs) == 1 and all(abs(p) <= 2 for p in piv)
    if name.startswith('SM_Trap_') and name != 'SM_Trap_CannonBall': ok = ok and max(dims) <= 100.01
    bad += not ok
    print('CHK %-20s %s tris=%4d pivot=(%.1f,%.1f,%.1f) min=(%.1f,%.1f,%.1f) max=(%.1f,%.1f,%.1f) dims=(%.1f,%.1f,%.1f)' % ((name, 'OK ' if ok else 'BAD', tris) + tuple(piv) + tuple(mn) + tuple(mx) + tuple(dims)))
print('FAILS', bad)
