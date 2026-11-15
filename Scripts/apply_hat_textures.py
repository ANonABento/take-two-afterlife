# Gives every SM_Hat_* a textured material from models/hat-tex/T_Hat_*.png.
# Run: py "C:/Users/kevbe/take-two-afterlife/Scripts/apply_hat_textures.py"
import os, unreal
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "models", "hat-tex")
HATS = "/Game/Characters/Hats"; TEX = HATS + "/Textures"; MAT = HATS + "/M_Hat_Textured"
MEL = unreal.MaterialEditingLibrary; EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
NAMES = ["Crown","VikingHelmet","PropellerBeanie","PartyCone","BunnyEars","DevilHorns","Halo","TrafficCone"]
tasks = []
for n in NAMES:
    f = os.path.join(SRC, "T_Hat_%s.png" % n)
    if not os.path.exists(f): unreal.log_warning("missing " + f); continue
    t = unreal.AssetImportTask(); t.filename = f; t.destination_path = TEX
    t.replace_existing = True; t.automated = True; t.save = True; tasks.append(t)
AT.import_asset_tasks(tasks)
first = EAL.load_asset(TEX + "/T_Hat_Crown")
if not EAL.does_asset_exist(MAT):
    m = AT.create_asset("M_Hat_Textured", HATS, unreal.Material, unreal.MaterialFactoryNew())
    node = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    node.set_editor_property("parameter_name", "Tex"); node.set_editor_property("texture", first)
    MEL.connect_material_property(node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m); EAL.save_asset(MAT)
mat = EAL.load_asset(MAT)
unreal.log("=== hat textures ===")
for n in NAMES:
    sm = EAL.load_asset("%s/SM_Hat_%s" % (HATS, n)); tex = EAL.load_asset("%s/T_Hat_%s" % (TEX, n))
    if not sm or not tex: unreal.log_warning("%s: mesh or texture missing" % n); continue
    mi_path = "%s/MI_Hat_%s" % (HATS, n)
    mi = EAL.load_asset(mi_path) if EAL.does_asset_exist(mi_path) else AT.create_asset(
        "MI_Hat_" + n, HATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, mat)
    MEL.set_material_instance_texture_parameter_value(mi, "Tex", tex)
    EAL.save_asset(mi_path)
    for i in range(sm.get_num_sections(0)): sm.set_material(i, mi)
    EAL.save_asset("%s/SM_Hat_%s" % (HATS, n)); unreal.log("%s: textured" % n)
