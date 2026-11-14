"""Engine-free reference check for the two map builders (art pass, task 1).

    python models/check_art_refs.py [--packs <folder with KayKitPrototypeBits/ KenneyCitySuburban/ KenneyNature/>]

Resolves what Scripts/build_backyard_map.py and models/build_heaven_map.py need before UE runs them:
their sibling-module imports (found the way each script extends sys.path), the Heaven FBX files and their
material slots, every palette colour name either map uses, and (with --packs) every kit FBX the backyard
layout references. Prints each missing reference and exits 1 if there are any.
"""
import ast
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SCRIPTS = REPO / "Scripts"
MODELS = REPO / "models"


def script_imports(path):
    """Top-level module names a script imports, minus stdlib/engine ones."""
    tree = ast.parse(path.read_text(encoding="utf-8"))
    names = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            names |= {a.name.split(".")[0] for a in node.names}
        elif isinstance(node, ast.ImportFrom) and node.module and node.level == 0:
            names.add(node.module.split(".")[0])
    return names - set(sys.stdlib_module_names) - {"unreal"}


def search_dirs(path):
    """Directories the script puts on sys.path: its own dir, plus Scripts/ if it inserts that."""
    text = path.read_text(encoding="utf-8")
    dirs = [path.parent]
    if re.search(r"parents\[1\]\s*/\s*\"Scripts\"", text):
        dirs.append(SCRIPTS)
    return dirs


def fbx_material_names(fbx):
    data = fbx.read_bytes()
    return {m.decode() for m in re.findall(rb"([A-Za-z_][A-Za-z0-9_]*)\x00\x01Material", data)}


def main(argv):
    missing = []
    for script in (SCRIPTS / "build_backyard_map.py", MODELS / "build_heaven_map.py"):
        if not script.is_file():
            missing.append(str(script.relative_to(REPO)))
            continue
        dirs = search_dirs(script)
        for mod in sorted(script_imports(script)):
            if not any((d / f"{mod}.py").is_file() for d in dirs):
                missing.append(f"{script.name}: import {mod} (searched {[str(d.relative_to(REPO)) for d in dirs]})")

    sys.path[:0] = [str(SCRIPTS), str(MODELS)]
    import toon_spec
    palette = {n for n, _ in toon_spec.PALETTE}

    heaven_src = (MODELS / "build_heaven_map.py").read_text(encoding="utf-8")
    meshes = re.search(r"MESHES = \(([^)]*)\)", heaven_src).group(1)
    slot_colour = ast.literal_eval(re.search(r"SLOT_COLOUR = (\{.*?\})", heaven_src, re.S).group(1))
    for name in re.findall(r"\"(\w+)\"", meshes):
        fbx = MODELS / f"{name}.fbx"
        if not fbx.is_file():
            missing.append(f"heaven mesh {fbx.relative_to(REPO)}")
            continue
        for slot in fbx_material_names(fbx):
            if slot not in slot_colour:
                missing.append(f"{name}: material slot {slot} has no SLOT_COLOUR entry")
    for slot, colour in slot_colour.items():
        if colour not in palette:
            missing.append(f"heaven SLOT_COLOUR {slot} -> {colour} not in toon_spec.PALETTE")

    import backyard_layout as L
    props = list(getattr(L, "PROPS", None) or L.layout() if hasattr(L, "layout") else getattr(L, "PROPS", []))
    for prop in props:
        if getattr(prop, "palette", None) and prop.palette not in palette:
            missing.append(f"backyard {prop.mesh}: palette {prop.palette} not in toon_spec.PALETTE")
    if "--packs" in argv:
        packs = Path(argv[argv.index("--packs") + 1])
        for prop in props:
            hits = list((packs / prop.kit).rglob(f"{prop.mesh}.fbx"))
            if not hits:
                missing.append(f"backyard kit mesh {prop.kit}/{prop.mesh}.fbx")
    print(f"checked {len(props)} backyard props, {len(slot_colour)} heaven slots")
    for m in missing:
        print("MISSING", m)
    print(f"{len(missing)} missing references")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
