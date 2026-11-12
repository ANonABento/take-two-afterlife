# Task 13: 6 job hats for SOCKET_Hat (angel head, socket at z=1.17, head top 1.20).
# Hat origin = socket point; +Z up, front = -Y. Palette UV (u=(i+.5)/12, v=.5) on shared T_TTA_Palette.
import bpy, bmesh, os, math, sys
OUT = os.path.dirname(os.path.abspath(__file__))
PAL = ["1E1B2E","F4F1E8","E5484D","F2913D","F5D547","6CC24A","2BB3A3","5BA8F5","3F5BD9","8E5CD9","F27BB0","9A6A43"]
bpy.ops.wm.read_factory_settings(use_empty=True)
def srgb(h):
    c=[int(h[i:i+2],16)/255 for i in (0,2,4)]
    return [x/12.92 if x<=0.04045 else ((x+0.055)/1.055)**2.4 for x in c]+[1]
pimg = bpy.data.images.new("T_TTA_Palette", 12, 1); px=[]
for h in PAL: px += srgb(h)
pimg.pixels = px
m = bpy.data.materials.new("MI_TTA_Toon_Hat"); m.use_nodes=True
bsdf = m.node_tree.nodes["Principled BSDF"]; t = m.node_tree.nodes.new("ShaderNodeTexImage")
t.image=pimg; t.interpolation='Closest'; m.node_tree.links.new(t.outputs[0], bsdf.inputs[0]); bsdf.inputs["Roughness"].default_value=0.8

def prim(kind, idx, loc=(0,0,0), scale=(1,1,1), rot=(0,0,0), **kw):
    getattr(bpy.ops.mesh, "primitive_%s_add" % kind)(location=loc, rotation=rot, **kw)
    o = bpy.context.object; o.scale = scale
    bpy.ops.object.transform_apply(scale=True, rotation=True, location=True)
    uv = o.data.uv_layers.new(name="UVMap")
    for l in uv.data: l.uv = ((idx+0.5)/12, 0.5)
    return o
def cyl(idx, r, d, z, seg=16, r2=None, x=0, y=0, sx=1, sy=1, rot=(0,0,0)):
    if r2 is None: return prim("cylinder", idx, (x,y,z+d/2), (sx,sy,1), rot, vertices=seg, radius=r, depth=d)
    return prim("cone", idx, (x,y,z+d/2), (sx,sy,1), rot, vertices=seg, radius1=r, radius2=r2, depth=d)
def sph(idx, r, loc, scale=(1,1,1), seg=12, ring=6):
    o = prim("uv_sphere", idx, loc, scale, segments=seg, ring_count=ring, radius=r); return o
def cube(idx, loc, size, rot=(0,0,0)): return prim("cube", idx, loc, size, rot, size=1)
def hemi(idx, r, z, sz=1.0, seg=16, ring=9):  # odd ring count: no vertex ring on the cut plane
    o = sph(idx, r, (0,0,z), (1,1,sz), seg, ring)
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:]+bm.edges[:]+bm.faces[:], plane_co=(0,0,z), plane_no=(0,0,1), clear_inner=True)
    edges=[e for e in bm.edges if e.is_boundary]; bmesh.ops.holes_fill(bm, edges=edges); bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(o.data); bm.free()
    uv=o.data.uv_layers[0]
    for l in uv.data: l.uv=((idx+0.5)/12,0.5)
    return o

R = 0.19  # crown radius clearing head (head r at socket ~0.12, at z=1.10 ~0.24 -> hats sit on top)
B = -0.03 # hat base below socket
HATS = {
 "HardHat": lambda: [hemi(4, 0.21, B, 0.85), cyl(4, 0.25, 0.02, B, 16, sy=1.0, y=-0.03), cube(4,(0,0,B+0.13),(0.04,0.30,0.06))],
 "Chef":    lambda: [cyl(1, 0.17, 0.12, B, 12), sph(1, 0.22, (0,0,B+0.24), (1,1,0.75), 12, 6)],
 "Police":  lambda: [cyl(8, 0.18, 0.08, B, 12), cyl(8, 0.18, 0.06, B+0.08, 12, r2=0.23, y=0.01),
                     cyl(0, 0.17, 0.015, B-0.005, 12, sx=0.9, sy=0.7, y=-0.12), cube(4,(0,-0.185,B+0.05),(0.05,0.02,0.05))],
 "Cowboy":  lambda: [cyl(11, 0.34, 0.025, B, 14, sx=1.0, sy=0.85), cyl(11, 0.17, 0.16, B+0.02, 10, r2=0.14), cyl(0, 0.175, 0.03, B+0.03, 10)],
 "Firefighter": lambda: [hemi(2, 0.2, B, 1.0), cyl(2, 0.27, 0.02, B, 14, sy=1.25, y=0.06), cube(4,(0,-0.19,B+0.08),(0.09,0.03,0.11)), cube(2,(0,0,B+0.19),(0.03,0.26,0.03))],
 "Pilot":   lambda: [cyl(3, 0.185, 0.09, B, 12, r2=0.16), cyl(3, 0.155, 0.04, B+0.09, 12),
                     cyl(0, 0.17, 0.015, B-0.005, 12, sx=0.9, sy=0.6, y=-0.14),
                     sph(7, 0.055, (0.07,-0.17,B+0.11),(1,0.6,1),8,4), sph(7, 0.055, (-0.07,-0.17,B+0.11),(1,0.6,1),8,4)],
}
stats = {}
for i,(name,fn) in enumerate(HATS.items()):
    parts = fn()
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts: p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]; bpy.ops.object.join()
    o = bpy.context.object; o.name = o.data.name = "SM_Hat_"+name
    o.data.materials.clear(); o.data.materials.append(m)
    bpy.ops.object.shade_flat()
    tris = sum(len(p.vertices)-2 for p in o.data.polygons); stats[name]=tris
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, o.name+".fbx"), use_selection=True, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE', path_mode='STRIP')
    o["display_x"] = (i-2.5)*0.7
print("TRIS", stats)
for o in bpy.data.objects: o.location.x = o["display_x"]
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT,"hats.glb"), export_format='GLB')
pimg.filepath_raw=os.path.join(OUT,"T_TTA_Palette.png"); pimg.file_format='PNG'; pimg.save()
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT,"hats.blend"))
# preview render
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.samples=32; sc.cycles.use_denoising=False
sc.render.resolution_x=800; sc.render.resolution_y=300; sc.view_settings.view_transform='Standard'
w=bpy.data.worlds.new("w"); w.color=(0.8,0.85,0.95); sc.world=w
w.use_nodes=True; w.node_tree.nodes["Background"].inputs[0].default_value=(0.75,0.82,0.95,1)
bpy.ops.object.camera_add(location=(0,-4.2,1.3), rotation=(math.radians(76),0,0)); c=bpy.context.object; sc.camera=c
c.data.type='ORTHO'; c.data.ortho_scale=4.4; c.location=(0,-5,1.6); c.rotation_euler=(math.radians(72),0,0)
bpy.ops.object.light_add(type='SUN', rotation=(math.radians(40),math.radians(20),math.radians(-30))); bpy.context.object.data.energy=4
sc.render.filepath=os.path.join(OUT,"hats_lineup.png"); bpy.ops.render.render(write_still=True)
c.location=(0,-5,0.12); c.rotation_euler=(math.radians(90),0,0)
sc.render.filepath=os.path.join(OUT,"hats_front.png"); bpy.ops.render.render(write_still=True)
