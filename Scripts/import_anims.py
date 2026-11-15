# Import the gen 3 angel animations into Unreal.
# Run in Output Log (Cmd = Python):
#   py "C:/Users/kevbe/take-two-afterlife/Scripts/import_anims.py"
# Looks for A_Angel_*.fbx in SEARCH_DIRS (first hit wins).
import os, unreal

SEARCH_DIRS = [
    os.path.join(os.path.expanduser("~"), "Downloads"),
    os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "models", "anims_gen3"),
]
NAMES = ["Idle", "Run", "JumpStart", "FallLoop", "Land", "Hit", "Cheer", "Sit"]
SKELETON = "/Game/Characters/Angel/SK_Angel_Skeleton"
DEST = "/Game/Characters/Angel/Anims"
ABP = "ABP_Angel"

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = []

skel = unreal.load_asset(SKELETON)
if not skel:
    raise RuntimeError("Skeleton not found: " + SKELETON)


def find_fbx(n):
    for d in SEARCH_DIRS:
        p = os.path.join(d, "A_Angel_%s.fbx" % n)
        if os.path.isfile(p):
            return p
    return None


def anims_in(folder):
    out = []
    if not lib.does_directory_exist(folder):
        return out
    for path in lib.list_assets(folder, recursive=False):
        a = unreal.load_asset(path)
        if isinstance(a, unreal.AnimSequence):
            out.append(path)
    return out


for n in NAMES:
    src = find_fbx(n)
    if not src:
        report.append("MISSING A_Angel_%s.fbx" % n)
        continue
    before = set(anims_in(DEST))
    ui = unreal.FbxImportUI()
    ui.import_mesh = False
    ui.import_materials = False
    ui.import_textures = False
    ui.import_as_skeletal = True
    ui.import_animations = True
    ui.skeleton = skel
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    ad = ui.anim_sequence_import_data
    ad.set_editor_property("import_bone_tracks", True)
    ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    ad.set_editor_property("remove_redundant_keys", False)
    t = unreal.AssetImportTask()
    t.filename = src
    t.destination_path = DEST
    t.destination_name = "A_Angel_" + n
    t.replace_existing = True
    t.automated = True
    t.save = True
    t.options = ui
    tools.import_asset_tasks([t])
    target = DEST + "/A_Angel_" + n
    if not lib.does_asset_exist(target):
        # Unreal may append the take name; rename the new sequence.
        new = [p for p in anims_in(DEST) if p not in before]
        if new:
            lib.rename_asset(new[0].split(".")[0], target)
    report.append(("OK " if lib.does_asset_exist(target) else "FAILED ") + target + "  <- " + src)

# Animation Blueprint shell. Unreal Python cannot build AnimGraph nodes; graph steps are printed below.
abp_path = DEST + "/" + ABP
if not lib.does_asset_exist(abp_path):
    f = unreal.AnimBlueprintFactory()
    f.set_editor_property("target_skeleton", skel)
    f.set_editor_property("parent_class", unreal.AnimInstance)
    abp = tools.create_asset(ABP, DEST, unreal.AnimBlueprint, f)
    report.append(("CREATED " if abp else "FAILED ") + abp_path)
else:
    report.append("EXISTS " + abp_path)

# 1D blend space for Idle -> Run by speed.
bs_path = DEST + "/BS_Angel_Locomotion"
if not lib.does_asset_exist(bs_path):
    try:
        bf = unreal.BlendSpaceFactory1D()
        bf.set_editor_property("target_skeleton", skel)
        bs = tools.create_asset("BS_Angel_Locomotion", DEST, unreal.BlendSpace1D, bf)
        report.append(("CREATED " if bs else "FAILED ") + bs_path)
    except Exception as e:
        report.append("SKIPPED blend space: %s" % e)

lib.save_directory(DEST, only_if_is_dirty=False, recursive=True)

print("=== gen 3 anims ===")
for r in report:
    print(r)
print("""Manual (about 5 min):
1. BS_Angel_Locomotion: horizontal axis Speed 0..600. Drop A_Angel_Idle at 0, A_Angel_Run at 600. Save.
2. ABP_Angel Event Graph: Try Get Pawn Owner -> Get Velocity -> VectorLengthXY -> set float Speed.
   Try Get Pawn Owner -> Get Movement Component -> Is Falling -> set bool IsInAir.
3. ABP_Angel AnimGraph: State Machine. Ground = BS_Angel_Locomotion(Speed). Air = A_Angel_FallLoop (loop).
   Ground->Air when IsInAir, Air->Ground when NOT IsInAir. Put a Slot 'DefaultSlot' before Output Pose
   so code can play Hit / Cheer / Sit / JumpStart / Land as montages.
4. On the pawn's mesh component (or SK_Angel post-process): Anim Class = ABP_Angel. Compile, save, Play.""")
