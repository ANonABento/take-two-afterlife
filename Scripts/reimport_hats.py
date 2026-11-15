import os, unreal
SRC = "C:/Users/kevbe/Downloads"
DEST = "/Game/Characters/Hats"
HATS = {"crown":"SM_Hat_Crown","viking-helmet":"SM_Hat_VikingHelmet","propeller-beanie":"SM_Hat_PropellerBeanie",
        "party-cone":"SM_Hat_PartyCone","bunny-ears":"SM_Hat_BunnyEars","devil-horns":"SM_Hat_DevilHorns",
        "halo":"SM_Hat_Halo","traffic-cone":"SM_Hat_TrafficCone"}
def opts():
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_as_skeletal = False
    o.import_materials = True; o.import_textures = True; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    sm = o.static_mesh_import_data
    sm.combine_meshes = True; sm.import_uniform_scale = 1.0
    sm.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    sm.auto_generate_collision = False
    return o
tasks = []
for src, name in HATS.items():
    f = os.path.join(SRC, src + ".fbx")
    if not os.path.exists(f): unreal.log_warning("missing " + f); continue
    t = unreal.AssetImportTask()
    t.filename = f; t.destination_path = DEST; t.destination_name = name
    t.replace_existing = True; t.automated = True; t.save = True; t.options = opts()
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
unreal.log("=== hat reimport ===")
for name in HATS.values():
    m = unreal.EditorAssetLibrary.load_asset(DEST + "/" + name)
    if not m: unreal.log_warning(name + ": NOT LOADED"); continue
    b = m.get_bounding_box(); s = b.max - b.min
    unreal.log("%s: %.1f x %.1f x %.1f cm, tris %s" % (name, s.x, s.y, s.z, m.get_num_triangles(0)))
