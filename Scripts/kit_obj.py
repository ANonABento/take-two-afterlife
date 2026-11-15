"""Engine-free OBJ fix-up for kits that ship metres, Y-up OBJ only (KayKit Prototype Bits).

UE's OBJ import applies no unit or up-axis conversion (it only mirrors Y), so those meshes arrive 100x too
small and lying on their side. `stage()` writes a copy that UE imports upright and in centimetres:
every position is rotated +90 degrees about X (Y-up -> Z-up after UE's Y mirror) and scaled.
"""
import shutil
from pathlib import Path

# kit key -> scale to centimetres. Kits not listed are imported as shipped (their FBX carries units and axes).
OBJ_FIX = {"KayKitPrototypeBits": 100.0}


def _rotate(x, y, z):
    return x, -z, y


def transform_obj(text, scale):
    """OBJ text with `v` rotated and scaled, `vn` rotated; everything else untouched."""
    out = []
    for line in text.splitlines():
        parts = line.split()
        if parts and parts[0] in ("v", "vn") and len(parts) >= 4:
            x, y, z = _rotate(*(float(v) for v in parts[1:4]))
            k = scale if parts[0] == "v" else 1.0
            rest = " ".join(parts[4:])
            line = f"{parts[0]} {x * k:.6f} {y * k:.6f} {z * k:.6f}" + (f" {rest}" if rest else "")
        out.append(line)
    return "\n".join(out) + "\n"


def ue_bounds(text):
    """((min x, y, z), (max x, y, z)) of an OBJ as UE imports it (Y mirrored)."""
    pts = [(float(p[1]), -float(p[2]), float(p[3]))
           for p in (line.split() for line in text.splitlines()) if p and p[0] == "v"]
    return tuple(min(c) for c in zip(*pts)), tuple(max(c) for c in zip(*pts))


def stage(kit_dir, staging_dir, scale):
    """Copy every OBJ under kit_dir (transformed), with its .mtl and textures, into staging_dir. Returns the OBJs."""
    kit_dir, staging_dir = Path(kit_dir), Path(staging_dir)
    if staging_dir.exists():
        shutil.rmtree(staging_dir)
    staging_dir.mkdir(parents=True)
    staged = []
    for src in sorted(kit_dir.rglob("*.obj")):
        if "(unity)" in str(src).lower():
            continue
        dst = staging_dir / src.name
        dst.write_text(transform_obj(src.read_text(encoding="utf-8"), scale), encoding="utf-8")
        staged.append(dst)
        for side in list(src.parent.glob("*.mtl")) + list(src.parent.glob("*.png")):
            if not (staging_dir / side.name).exists():
                shutil.copy2(side, staging_dir / side.name)
    return staged
