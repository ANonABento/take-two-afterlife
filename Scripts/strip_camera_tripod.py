# Drop the tripod off the placeable camera. It is spawned in the air, so the legs hang in
# an empty gap. The body (about the top 11 cm of the 40 cm prop) is kept and shifted down
# onto the mesh pivot.
#
# Writes /Game/Art/Cams/SM_Cam_FixedBody. The source SM_Cam_Fixed stays as the FBX import,
# which still has legs until models/build_traps_cams.py is re-exported. A second editor
# often has SM_Cam_Fixed locked, so the stripped mesh is a separate asset.
#
#   UnrealEditor-Cmd TakeTwoAfterlife.uproject -EnablePlugins=GeometryScripting -run=pythonscript -script=Scripts/strip_camera_tripod.py -unattended -nosplash
import unreal

SOURCE = "/Game/Art/Cams/SM_Cam_Fixed"
DEST = "/Game/Art/Cams/SM_Cam_FixedBody"
# Body box is 10 cm tall. Anything below the top 11 cm is tripod.
BODY_KEEP_CM = 11.0


def _as_tuple(result):
    return result if isinstance(result, tuple) else (result,)


def _stripped(source):
    dynamic = unreal.DynamicMesh()
    dynamic = _as_tuple(unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        source, dynamic, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD()))[0]
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dynamic)
    height = box.max.z - box.min.z
    unreal.log(f"{SOURCE} height {height:.1f} cm  z {box.min.z:.1f}..{box.max.z:.1f}")
    if height <= 20.0:
        return dynamic
    cut = box.max.z - BODY_KEEP_CM
    below = unreal.Box(
        unreal.Vector(box.min.x - 10.0, box.min.y - 10.0, box.min.z - 10.0),
        unreal.Vector(box.max.x + 10.0, box.max.y + 10.0, cut))
    # One vertex inside the box is enough, so the neck under the body goes too.
    # Selection comes back as a return value; UE Python does not take it as an argument.
    picked = _as_tuple(unreal.GeometryScript_MeshSelection.select_mesh_elements_in_box(
        dynamic, below, unreal.GeometryScriptMeshSelectionType.TRIANGLES, False, 1))
    dynamic, selection = picked[0], picked[1]
    deleted = _as_tuple(unreal.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dynamic, selection))
    dynamic = deleted[0]
    removed = deleted[1] if len(deleted) > 1 else -1
    unreal.log(f"{SOURCE} removed {removed} tripod triangles")
    if isinstance(removed, int) and removed <= 0:
        raise RuntimeError(f"{SOURCE} still has its tripod (deleted {removed})")
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dynamic)
    if box.min.z > 0.5:
        dynamic = _as_tuple(unreal.GeometryScript_MeshTransforms.translate_mesh(
            dynamic, unreal.Vector(0.0, 0.0, -box.min.z)))[0]
        box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dynamic)
        unreal.log(f"{DEST} body z {box.min.z:.1f}..{box.max.z:.1f}")
    return dynamic


def strip_placed_cameras():
    source = unreal.EditorAssetLibrary.load_asset(SOURCE)
    if source is None:
        raise RuntimeError(f"missing {SOURCE}")
    dynamic = _stripped(source)
    if not unreal.EditorAssetLibrary.does_asset_exist(DEST):
        dest = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DEST)
        if dest is None:
            raise RuntimeError(f"could not duplicate {SOURCE} to {DEST}")
    else:
        dest = unreal.EditorAssetLibrary.load_asset(DEST)
    options = unreal.GeometryScriptCopyMeshToAssetOptions()
    options.emit_transaction = False
    options.replace_materials = False
    unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
        dynamic, dest, options, unreal.GeometryScriptMeshWriteLOD())
    if not unreal.EditorAssetLibrary.save_asset(DEST, only_if_is_dirty=False):
        raise RuntimeError(f"could not save {DEST}")
    unreal.log(f"{DEST} saved")


if __name__ == "__main__":
    strip_placed_cameras()
