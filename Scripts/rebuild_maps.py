# Run inside the open UE editor (Output Log, input box set to Python), full path:
#   C:/Users/<you>/take-two-afterlife/Scripts/rebuild_maps.py          maps only: Backyard, Heaven, look volumes
#   C:/Users/<you>/take-two-afterlife/Scripts/rebuild_maps.py full     meshes first: our models, KayKit, toon swap
# One entry point for the map pipeline, in the only order that works:
#   import_mac_assets -> import_kits (KayKit) -> fix_prototype_material -> swap_to_toon       (full only)
#   -> build_backyard_map -> dress_heaven -> apply_world_look
# swap_to_toon comes before the maps (Backyard keeps the Kenney kit's swapped materials) and apply_world_look
# comes last (it re-saves every map). Stops at the first script that raises, and says which.
# Not headless: the map builders destroy and spawn level actors, which crashes UnrealEditor-Cmd -run=pythonscript.
# Character scripts (import_gen3_assets, import_anims, import_faces, reimport_hats, build_cosmetics_table) and
# create_toon_assets / create_title_map / import_fonts / import_ui_assets are separate one-offs and are not run here.
import os
import runpy
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
SHARED = ("backyard_layout", "layout_spawn", "kit_obj", "maplib", "toon_spec", "hand_dressed")   # cached by the editor
MESH_STEPS = (("import_mac_assets.py", ()), ("import_kits.py", ("KayKitPrototypeBits",)),
              ("fix_prototype_material.py", ()), ("swap_to_toon.py", ()))
MAP_STEPS = (("build_backyard_map.py", ()), ("dress_heaven.py", ()), ("apply_world_look.py", ()))


def steps(args):
    """Scripts to run for these command-line args, in order."""
    if args not in ([], ["full"]):
        raise SystemExit("usage: rebuild_maps.py [full]")
    return (MESH_STEPS if args else ()) + MAP_STEPS


def forget_shared():
    """The editor keeps imported modules for its whole session: drop ours so edited scripts are re-read."""
    for name in [m for m in sys.modules if m.split(".")[0] in SHARED]:
        del sys.modules[name]


def run(args):
    todo = steps(args)
    argv = sys.argv
    try:
        for index, (name, script_args) in enumerate(todo, 1):
            forget_shared()
            path = os.path.join(HERE, name)
            unreal.log_warning(f"rebuild_maps {index}/{len(todo)}: {name} {' '.join(script_args)}")
            sys.argv = [path, *script_args]
            try:
                runpy.run_path(path, run_name="__main__")
            except BaseException as err:   # SystemExit too: a script that bails must stop the chain
                unreal.log_error(f"rebuild_maps stopped at {name}: {type(err).__name__}: {err}")
                raise
    finally:
        sys.argv = argv
    unreal.log_warning(f"rebuild_maps done: {', '.join(name for name, _ in todo)}")


if __name__ == "__main__":
    run(sys.argv[1:])
