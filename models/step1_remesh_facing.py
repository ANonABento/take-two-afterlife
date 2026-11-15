import bpy, bmesh, json, os, math
from mathutils import Vector
W='/workspace' if os.path.exists('/workspace/models') else os.getcwd()
OUT=os.path.join(W,'models/hatfit/v7'); os.makedirs(OUT,exist_ok=True)
res={}
# --- facing from the angel's head geometry
bpy.ops.wm.open_mainfile(filepath=os.path.join(W,'models/angel-v2/gen3/final/angel.blend'))
meshes=[o for o in bpy.data.objects if o.type=='MESH']
dg=bpy.context.evaluated_depsgraph_get()
pts=[];mats={}
for o in meshes:
    e=o.evaluated_get(dg); m=e.to_mesh()
    for p in m.polygons:
        c=o.matrix_world@p.center
        if c.z>0.55:
            pts.append(c)
            mn=o.material_slots[p.material_index].name if o.material_slots else '-'
            mats.setdefault(mn,[]).append(c)
    e.to_mesh_clear()
n=len(pts); cx=sum(p.x for p in pts)/n; cy=sum(p.y for p in pts)/n
res['head_centre']=[cx,cy]
res['extent']={'+X':max(p.x for p in pts)-cx,'-X':cx-min(p.x for p in pts),'+Y':max(p.y for p in pts)-cy,'-Y':cy-min(p.y for p in pts)}
res['materials']={k:{'n':len(v),'meanX':sum(p.x for p in v)/len(v)-cx,'meanY':sum(p.y for p in v)/len(v)-cy,'meanZ':sum(p.z for p in v)/len(v)} for k,v in mats.items()}
# --- remesh hats into closed shells
for h in ['propeller-beanie','viking-helmet','bunny-ears','devil-horns']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(W,f'models/angel-v2/hats/{h}/{h}.glb'))
    ms=[o for o in bpy.context.scene.objects if o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in ms:o.select_set(True)
    bpy.context.view_layer.objects.active=ms[0]
    if len(ms)>1: bpy.ops.object.join()
    o=bpy.context.view_layer.objects.active
    bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    md=o.modifiers.new('r','REMESH'); md.mode='VOXEL'; md.voxel_size=0.003
    bpy.ops.object.modifier_apply(modifier='r')
    dc=o.modifiers.new('d','DECIMATE'); dc.ratio=min(1.0,20000/max(1,len(o.data.polygons)))
    bpy.ops.object.modifier_apply(modifier='d')
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.ops.export_mesh.stl(filepath=os.path.join(OUT,f'{h}_shell.stl'),use_selection=True)
    d=o.dimensions
    res[h]={'tris_after':len(o.data.polygons),'dims_xyz':[round(d.x,3),round(d.y,3),round(d.z,3)]}
json.dump(res,open(os.path.join(OUT,'step1.json'),'w'),indent=1)
print(json.dumps(res,indent=1))
