# Task 9 patch: stretch the Face slot's UV island to fill 0-1 (smart project had left it at 0.01-0.69 x 0.22-0.95),
# re-bake angel_face from the Face slot colour, re-export and re-render.
# blender -b final/angel.blend --python-exit-code 1 --python fix_face_uv.py -- <out_dir>
import bpy, sys, os, math, mathutils
OUT = os.path.abspath(sys.argv[sys.argv.index("--")+1]); os.makedirs(OUT, exist_ok=True)
ang = bpy.data.objects["Angel"]; rig = bpy.data.objects["AngelRig"]; me = ang.data
FACE = [i for i, m in enumerate(me.materials) if m.name == "Face"][0]
uv = me.uv_layers[0].data; L = [li for p in me.polygons if p.material_index == FACE for li in p.loop_indices]
mn = [min(uv[l].uv[i] for l in L) for i in (0, 1)]; mx = [max(uv[l].uv[i] for l in L) for i in (0, 1)]
for l in L: uv[l].uv = ((uv[l].uv[0]-mn[0])/(mx[0]-mn[0]), (uv[l].uv[1]-mn[1])/(mx[1]-mn[1]))
print("FACE_UV_BBOX", [round(min(uv[l].uv[i] for l in L), 3) for i in (0, 1)], [round(max(uv[l].uv[i] for l in L), 3) for i in (0, 1)])
src = ang.copy(); src.data = me.copy(); src.modifiers.clear(); src.parent = None; src.matrix_world = ang.matrix_world
bpy.context.scene.collection.objects.link(src)
for i, m in enumerate(src.data.materials):
    c = m.copy(); src.data.materials[i] = c; b = c.node_tree.nodes["Principled BSDF"].inputs["Base Color"]
    for lk in list(b.links): c.node_tree.links.remove(lk)
dummy = bpy.data.images.new("dummy", 8, 8)
for m in me.materials:
    t = m.node_tree.nodes["BakeTex"]
    if m.name != "Face": t2 = m.node_tree.nodes.new("ShaderNodeTexImage"); t2.image = dummy; m.node_tree.nodes.active = t2
    else: m.node_tree.nodes.active = t
sc = bpy.context.scene; sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 1
bk = sc.render.bake; bk.use_selected_to_active = True; bk.cage_extrusion = 0.005; bk.max_ray_distance = 0.02; bk.margin = 8
rig.data.pose_position = 'REST'
bpy.ops.object.select_all(action='DESELECT'); src.select_set(True); ang.select_set(True); bpy.context.view_layer.objects.active = ang
bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'})
face = bpy.data.images["angel_face"]; face.filepath_raw = os.path.join(OUT, "angel_face.png"); face.file_format = 'PNG'; face.save(); face.pack()
for m in me.materials:
    for n in list(m.node_tree.nodes):
        if n.type == 'TEX_IMAGE' and n.image == dummy: m.node_tree.nodes.remove(n)
bpy.data.objects.remove(src); bpy.data.images.remove(dummy)
for m in list(bpy.data.materials):
    if m.users == 0: bpy.data.materials.remove(m)
rig.data.pose_position = 'POSE'
body = bpy.data.images["angel_basecolor"]
print("ANGEL tris", sum(len(p.vertices)-2 for p in me.polygons), "uv", [u.name for u in me.uv_layers], "mats", [m.name for m in me.materials],
      "imgs", [(i.name, i.size[:]) for i in (body, face)], "head_hat", "head_hat" in rig.data.bones, "smooth", all(p.use_smooth for p in me.polygons), me.use_auto_smooth, round(math.degrees(me.auto_smooth_angle)))
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, 'angel.fbx'), use_selection=True, apply_scale_options='FBX_SCALE_UNITS',
    object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=False, mesh_smooth_type='FACE',
    primary_bone_axis='Y', secondary_bone_axis='X', path_mode='COPY', embed_textures=True)
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, 'angel.glb'), export_format='GLB')
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'angel.blend'), compress=True)
# renders
fc = sum((ang.matrix_world @ me.vertices[p.vertices[0]].co for p in me.polygons if p.material_index == FACE), mathutils.Vector())
nf = sum(1 for p in me.polygons if p.material_index == FACE); fc /= nf
bb = [ang.matrix_world @ mathutils.Vector(c) for c in ang.bound_box]; ctr = sum(bb, mathutils.Vector()) / 8
fwd = mathutils.Vector((fc.x - ctr.x, fc.y - ctr.y, 0)).normalized()
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (0.75, 0.82, 0.9, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 0.5
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.data.energy = 2.5; sc.collection.objects.link(sun)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 1.25
sc.cycles.samples = 32; sc.cycles.use_denoising = False; sc.render.resolution_x = sc.render.resolution_y = 640
sc.view_settings.view_transform = 'Standard'; sc.view_settings.look = 'Medium High Contrast'
for name, ang_deg in (("front", 0), ("side", 90), ("back", 180)):
    d = mathutils.Matrix.Rotation(math.radians(ang_deg), 3, 'Z') @ fwd
    cam.location = ctr + d * 3 + mathutils.Vector((0, 0, 0.1))
    cam.rotation_euler = (ctr - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sun.rotation_euler = (cam.location - ctr).to_track_quat('Z', 'Y').to_euler(); sun.rotation_euler.x += 0.4
    sc.render.filepath = os.path.join(OUT, f"render_{name}.png"); bpy.ops.render.render(write_still=True)
print("DONE")
