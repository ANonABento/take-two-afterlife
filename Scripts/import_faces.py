"""Re-import the drawn gen 3 faces onto the existing T_Face_* textures.
Keeps every reference (MI_Angel_Face, DT_Cosmetics) intact.
Run in Unreal (Output Log, Python):
  py "C:/Users/kevbe/take-two-afterlife/Scripts/import_faces.py"
"""
import os
import unreal

SRC_DIR = os.environ.get("TTA_FACE_DIR", r"C:\Users\kevbe\Downloads")
DEST = "/Game/Characters/Faces"
FACES = ["Smug", "Shocked", "Dizzy", "Angry", "Wink", "Sleepy", "Happy"]

tools = unreal.AssetToolsHelpers.get_asset_tools()
done, missing = [], []
for name in FACES:
    src = os.path.join(SRC_DIR, "T_Face_%s.png" % name)
    if not os.path.isfile(src):
        missing.append(name)
        continue
    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = DEST
    task.destination_name = "T_Face_" + name
    task.replace_existing = True
    task.automated = True
    task.save = False
    tools.import_asset_tasks([task])
    tex = unreal.load_asset("%s/T_Face_%s" % (DEST, name))
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("srgb", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
        done.append(name)

print("=== faces ===")
print("imported:", done)
print("no PNG found in %s for:" % SRC_DIR, missing)
