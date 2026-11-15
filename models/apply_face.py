# blender -b --factory-startup --python-exit-code 1 --python apply_face.py -- angel.glb T_Face_Smug.png out.png
import bpy, sys, os
from mathutils import Vector
argv = sys.argv[sys.argv.index('--')+1:]
glb, tex, out = [os.path.abspath(p) for p in argv]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb)
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
img = bpy.data.images.load(tex)
face_centres = []
for ob in meshes:
    for i, slot in enumerate(ob.material_slots):
        m = slot.material
        print('slot', ob.name, i, m.name if m else None)
        if not m or 'face' not in m.name.lower(): continue
        nt = m.node_tree; bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        base_in = bsdf.inputs['Base Color']
        if base_in.links:
            prev = base_in.links[0].from_socket
        else:
            rgb = nt.nodes.new('ShaderNodeRGB'); rgb.outputs[0].default_value = base_in.default_value; prev = rgb.outputs[0]
        t = nt.nodes.new('ShaderNodeTexImage'); t.image = img; t.name = 'FaceDecal'
        mix = nt.nodes.new('ShaderNodeMixRGB'); mix.blend_type = 'MIX'
        nt.links.new(t.outputs['Alpha'], mix.inputs['Fac'])
        nt.links.new(prev, mix.inputs['Color1'])
        nt.links.new(t.outputs['Color'], mix.inputs['Color2'])
        nt.links.new(mix.outputs[0], base_in)
        mw = ob.matrix_world
        for p in ob.data.polygons:
            if p.material_index == i: face_centres.append(mw @ p.center)
        uvl = ob.data.uv_layers.active.data
        us = [uvl[li].uv for p in ob.data.polygons if p.material_index == i for li in p.loop_indices]
        print('FACE polys', sum(p.material_index == i for p in ob.data.polygons),
              'UV x', min(u.x for u in us), max(u.x for u in us), 'y', min(u.y for u in us), max(u.y for u in us))
assert face_centres, 'no Face slot'
# bounds
pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
print('DIMS', tuple(round(v, 3) for v in (hi - lo)))
fc = sum(face_centres, Vector()) / len(face_centres)
centre = (lo + hi) / 2
d = Vector((fc.x - centre.x, fc.y - centre.y, 0)).normalized()
print('FACE centre', tuple(round(v,3) for v in fc), 'front dir', tuple(round(v,2) for v in d))
sc = bpy.context.scene
cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam')); sc.collection.objects.link(cam)
tgt = Vector((centre.x, centre.y, centre.z))
cam.location = tgt + d * 3.2
cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
cam.data.lens = 50; sc.camera = cam
sun = bpy.data.objects.new('Sun', bpy.data.lights.new('Sun', 'SUN')); sun.data.energy = 2.5
sc.collection.objects.link(sun); sun.rotation_euler = (cam.rotation_euler.x - 0.4, 0, cam.rotation_euler.z + 0.4)
w = bpy.data.worlds.new('W'); sc.world = w; w.use_nodes = True
w.node_tree.nodes['Background'].inputs[0].default_value = (0.8, 0.85, 0.95, 1); w.node_tree.nodes['Background'].inputs[1].default_value = 0.6
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 32; sc.cycles.use_denoising = False
sc.view_settings.view_transform = 'Standard'
sc.render.resolution_x = sc.render.resolution_y = 640
sc.render.filepath = out
bpy.ops.render.render(write_still=True)
print('RENDERED', out)
