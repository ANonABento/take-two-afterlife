# Render all 8 hats on the angel, 3/4 view, one image.
# blender -b --factory-startup --python-exit-code 1 --python render_hats_on_angel.py -- <dir with angel.glb + hat glbs> <out.png>
import bpy, sys, os, math, json, mathutils
a = sys.argv[sys.argv.index("--")+1:]; D, OUTP = os.path.abspath(a[0]), os.path.abspath(a[1])
fit = json.load(open(os.path.join(D, "hat_fit.json")))
HATS = ["halo","crown","propeller-beanie","viking-helmet","party-cone","traffic-cone","devil-horns","bunny-ears"]
SOCKET_Z = 1.0  # head_hat socket height measured on angel.glb
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
def imp(path, off):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    for o in set(bpy.data.objects) - before:
        if o.parent is None: o.location += off
for i, h in enumerate(HATS):
    off = mathutils.Vector((i * 0.68, i * 0.68, 0))  # row perpendicular to the 3/4 view
    imp(os.path.join(D, "angel.glb"), off)
    imp(os.path.join(D, h + ".glb"), off + mathutils.Vector((0, 0, SOCKET_Z + fit[h]["sit_offset_from_head_hat_m"])))
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 24; sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y = 800, 380
sc.view_settings.view_transform = 'Standard'; sc.view_settings.look = 'Medium High Contrast'
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.88, 1, 1)
w.node_tree.nodes["Background"].inputs[1].default_value = 0.5
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); sun.data.energy = 2.5
sun.rotation_euler = (math.radians(45), 0, math.radians(35)); sc.collection.objects.link(sun)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 7.6
c = mathutils.Vector((2.38, 2.38, 0.62))
cam.location = c + mathutils.Vector((1, -1, 0.8)).normalized() * 12
cam.rotation_euler = (c - cam.location).to_track_quat('-Z', 'Y').to_euler()
sc.render.filepath = OUTP; bpy.ops.render.render(write_still=True)
