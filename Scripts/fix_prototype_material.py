"""UE 5.5 editor script: KayKit prototype_texture is Translucent, which Nanite meshes
(Barrel_A etc.) can't render. Set it to Masked. Run in the editor: py Scripts/fix_prototype_material.py"""
import unreal

ROOT = "/Game/ThirdParty/KayKitPrototypeBits"
NAME = "prototype_texture"


def fix_prototype_materials(root=ROOT):
    changed = []
    for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
        if path.rsplit("/", 1)[-1].split(".")[0].lower() != NAME:
            continue
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Material):
            if asset.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_MASKED:
                asset.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
                unreal.MaterialEditingLibrary.recompile_material(asset)
                changed.append(path)
        elif isinstance(asset, unreal.MaterialInstanceConstant):
            ov = asset.get_editor_property("base_property_overrides")
            ov.set_editor_property("override_blend_mode", True)
            ov.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
            asset.set_editor_property("base_property_overrides", ov)
            changed.append(path)
        else:
            continue
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
        unreal.log(f"fix_prototype_material: {path} -> Masked")
    if not changed:
        unreal.log_warning(f"fix_prototype_material: nothing changed under {root}")
    return changed


if __name__ == "__main__":
    fix_prototype_materials()
