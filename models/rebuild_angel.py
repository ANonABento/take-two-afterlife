# Task 9: fix the angel's cracked look. The Tripo angel gen had texture=False, so there is no raw colour
# texture; rebuild from the current final/angel.blend (same 9.1k-tri skinned mesh, rig, head_hat bone).
# Wings joined into Angel (weighted 100% to Spine2), Smart UV on Body/Outfit/wing faces (Face island kept 0-1),
# Cycles bake of the slot colours (DIFFUSE/COLOR, selected-to-active from an untouched copy) into
# angel_basecolor 2048 (Body+Outfit) and angel_face 1024 (Face), Smooth by Angle 30 deg, re-export, render.
# blender -b final/angel.blend --python-exit-code 1 --python rebuild_angel.py -- <out_dir>
import bpy, sys, os, math, mathutils
OUT = os.path.abspath(sys.argv[sys.argv.index("--")+1]); os.makedirs(OUT, exist_ok=True)
ang = bpy.data.objects["Angel"]; rig = bpy.data.objects["AngelRig"]
# wings -> Angel
for wn in ("Wing_L", "Wing_R"):
    w = bpy.data.objects.get(wn)
    if not w: continue
    mw = w.matrix_world.copy(); w.parent = None; w.matrix_world = mw
    bpy.ops.object.select_all(action='DESELECT'); w.select_set(True); bpy.context.view_layer.objects.active = w
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    vg = w.vertex_groups.new(name="Spine2"); vg.add([v.index for v in w.data.vertices], 1.0, 'REPLACE')
    bpy.ops.object.select_all(action='DESELECT'); w.select_set(True); ang.select_set(True); bpy.context.view_layer.objects.active = ang
    bpy.ops.object.join()
me = ang.data
while len(me.uv_layers) > 1: me.uv_layers.remove(me.uv_layers[-1])
me.uv_layers[0].name = "UVMap"
FACE = [i for i, m in enumerate(me.materials) if m.name == "Face"][0]
# source copy with the original flat-colour slots
src = ang.copy(); src.data = me.copy(); src.modifiers.clear(); src.parent = None; src.matrix_world = ang.matrix_world
bpy.context.scene.collection.objects.link(src)
for i, m in enumerate(src.data.materials): src.data.materials[i] = m.copy()
# Smart UV on everything except the Face slot
bpy.ops.object.select_all(action='DESELECT'); ang.select_set(True); bpy.context.view_layer.objects.active = ang
for p in me.polygons: p.select = p.material_index != FACE
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.005)
bpy.ops.object.mode_set(mode='OBJECT')
uv = me.uv_layers[0].data; fu = [uv[li].uv for p in me.polygons if p.material_index == FACE for li in p.loop_indices]
print("FACE_UV_BBOX", [round(min(u[i] for u in fu), 3) for i in (0, 1)], [round(max(u[i] for u in fu), 3) for i in (0, 1)])
body = bpy.data.images.new("angel_basecolor", 2048, 2048); face = bpy.data.images.new("angel_face", 1024, 1024)
for m in me.materials:
    nt = m.node_tree; t = nt.nodes.new("ShaderNodeTexImage"); t.name = "BakeTex"
    t.image = face if m.name == "Face" else body; nt.nodes.active = t
sc = bpy.context.scene; sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 1
bk = sc.render.bake; bk.use_selected_to_active = True; bk.cage_extrusion = 0.005; bk.max_ray_distance = 0.02; bk.margin = 8
rig.data.pose_position = 'REST'
bpy.ops.object.select_all(action='DESELECT'); src.select_set(True); ang.select_set(True); bpy.context.view_layer.objects.active = ang
bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'})
for img in (body, face):
    img.filepath_raw = os.path.join(OUT, img.name + ".png"); img.file_format = 'PNG'; img.save(); img.pack()
for m in me.materials:
    nt = m.node_tree; nt.links.new(nt.nodes["BakeTex"].outputs["Color"], nt.nodes["Principled BSDF"].inputs["Base Color"])
bpy.data.objects.remove(src)
bpy.ops.object.select_all(action='DESELECT'); ang.select_set(True); bpy.context.view_layer.objects.active = ang
bpy.ops.object.shade_smooth(); me.use_auto_smooth = True; me.auto_smooth_angle = math.radians(30)
rig.data.pose_position = 'POSE'
print("ANGEL tris", sum(len(p.vertices)-2 for p in me.polygons), "uv", [u.name for u in me.uv_layers], "mats", [m.name for m in me.materials],
      "imgs", [(i.name, i.size[:]) for i in (body, face)], "head_hat", "head_hat" in rig.data.bones, "objs", [o.name for o in bpy.data.objects])
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
