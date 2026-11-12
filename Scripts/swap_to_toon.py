# Run inside the UE editor (PythonScriptPlugin):
#   py Scripts/swap_to_toon.py /Game/Imported/Kenney [/Game/Imported/KayKit ...]
# Every static/skeletal mesh under the folders gets each material slot that is not already an M_TTA_Toon
# instance replaced by MI_TTA_Toon_<material>: PaletteIndex = nearest palette colour to the old material's
# colour parameter (Bone when it has none); UsePaletteUV = 1 when the old material samples a texture
# (meshes exported with palette UVs).
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import toon_spec  # noqa: E402

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
COLOUR_PARAMS = ("BaseColor", "Base Color", "Color", "Colour", "Tint", "Albedo")


def is_toon(material):
    return (isinstance(material, unreal.MaterialInstance)
            and material.get_base_material().get_path_name().startswith(toon_spec.TOON_MATERIAL + "."))


def source_colour(material):
    if not isinstance(material, unreal.MaterialInstance):
        return None
    for p in material.get_editor_property("vector_parameter_values"):
        if str(p.parameter_info.name) in COLOUR_PARAMS:
            v = p.parameter_value
            return tuple(round(min(max(c, 0.0), 1.0) ** (1 / 2.2) * 255) for c in (v.r, v.g, v.b))
    return None


def has_texture(material):
    return isinstance(material, unreal.MaterialInstance) and bool(
        material.get_editor_property("texture_parameter_values"))


def toon_instance(old, toon):
    name = toon_spec.instance_name(old.get_name())
    path = f"{toon_spec.INSTANCE_DIR}/{name}"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, toon_spec.INSTANCE_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if mi is None:
        raise RuntimeError(f"could not create {path}")
    MEL.set_material_instance_parent(mi, toon)
    colour = source_colour(old)
    index = toon_spec.DEFAULT_INDEX if colour is None else toon_spec.nearest_palette_index(colour)
    MEL.set_material_instance_scalar_parameter_value(mi, "PaletteIndex", float(index))
    MEL.set_material_instance_scalar_parameter_value(mi, "UsePaletteUV", 1.0 if has_texture(old) else 0.0)
    EAL.save_loaded_asset(mi)
    return mi


def swap(folders):
    toon = EAL.load_asset(toon_spec.TOON_MATERIAL)
    if toon is None:
        raise RuntimeError(f"{toon_spec.TOON_MATERIAL} missing: run Scripts/create_toon_assets.py first")
    swapped = 0
    for folder in folders:
        for path in EAL.list_assets(folder, recursive=True, include_folder=False):
            asset = EAL.load_asset(path)
            if isinstance(asset, unreal.StaticMesh):
                prop = "static_materials"
            elif isinstance(asset, unreal.SkeletalMesh):
                prop = "materials"
            else:
                continue
            slots = asset.get_editor_property(prop)
            changed = 0
            for slot in slots:
                old = slot.get_editor_property("material_interface")
                if old is None or is_toon(old):
                    continue
                slot.set_editor_property("material_interface", toon_instance(old, toon))
                changed += 1
            if changed:
                asset.set_editor_property(prop, slots)
                EAL.save_loaded_asset(asset)
                swapped += changed
    unreal.log(f"swapped {swapped} material slots to M_TTA_Toon instances")


if len(sys.argv) < 2:
    raise SystemExit("usage: swap_to_toon.py <content folder> [...]")
swap(sys.argv[1:])
