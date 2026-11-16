"""Engine-free trap size/pivot rules shared by verify_traps_cams.py (Blender) and the unittest suite.

Boxes live in trap_bounds.json (cm, Blender Z-up, relative to the pivot). Because the box is stored
relative to the pivot, a mesh whose pivot drifts off the plan.md convention also moves outside its box.
"""
import json, os, re, struct

BOUNDS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'trap_bounds.json')
THIRDPARTY_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'thirdparty', 'traps')
PALETTE_MD = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'palette.md')
# CC0 trap models picked in trap-swap task 1 (Poly Pizza mirrors of Quaternius / Kenney).
# name -> (file in thirdparty/traps, download URL, palette colour). Traps not listed stay procedural.
THIRDPARTY = {
    'SM_Trap_Trapdoor': ('trapdoor_quaternius.glb', 'https://static.poly.pizza/49fcaac0-bf6b-4b8d-bf6a-67b55b8d99f6.glb', 'Wood'),
    'SM_Trap_Banana': ('banana_kenney.glb', 'https://static.poly.pizza/bd78ebc9-2834-45d5-af16-e00f604bdf36.glb', 'Yellow'),
    'SM_Trap_Hammer': ('hammer_quaternius.glb', 'https://static.poly.pizza/97f342bf-7e3b-4155-a306-91b35e321c7b.glb', 'Wood'),
    'SM_Trap_Spinner': ('sawblade_quaternius.glb', 'https://static.poly.pizza/943d61f9-8656-4b25-8782-c57b71be3e1b.glb', 'Red'),
}
TOLERANCE = 0.10      # +-10% of the recorded size, per axis
SLACK_CM = 0.5        # float/export rounding, so flat axes (0 cm) still compare
PIVOT_CM = 2.0        # object origin must sit at the world origin


def load_bounds(path=BOUNDS_PATH):
    with open(path, encoding='utf-8') as fh:
        return {k: v for k, v in json.load(fh).items() if not k.startswith('_')}


def check_trap(name, mn, mx, pivot, bounds):
    """Return a list of problems (empty = OK) for one exported trap, all values in cm."""
    if name not in bounds:
        return ['no recorded box in trap_bounds.json']
    box = bounds[name]
    problems = []
    if any(abs(p) > PIVOT_CM for p in pivot):
        problems.append('pivot %s not at origin' % (tuple(round(p, 1) for p in pivot),))
    for i, axis in enumerate('XYZ'):
        size = box['max_cm'][i] - box['min_cm'][i]
        tol = TOLERANCE * size + SLACK_CM
        dim = mx[i] - mn[i]
        if abs(dim - size) > tol:
            problems.append('%s size %.1f cm vs box %.1f +-%.1f' % (axis, dim, size, tol))
        if abs(mn[i] - box['min_cm'][i]) > tol or abs(mx[i] - box['max_cm'][i]) > tol:
            problems.append('%s extent [%.1f, %.1f] off box [%.1f, %.1f] (pivot convention)'
                            % (axis, mn[i], mx[i], box['min_cm'][i], box['max_cm'][i]))
    return problems


def glb_bounds(path):
    """{node name: (min_cm, max_cm)} for each mesh node in a .glb, in Blender Z-up axes, relative to the node origin."""
    with open(path, 'rb') as fh:
        data = fh.read()
    gltf = json.loads(data[20:20 + struct.unpack('<I', data[12:16])[0]])
    out = {}
    for node in gltf['nodes']:
        if 'mesh' not in node:
            continue
        acc = [gltf['accessors'][p['attributes']['POSITION']] for p in gltf['meshes'][node['mesh']]['primitives']]
        lo = [min(a['min'][i] for a in acc) for i in range(3)]
        hi = [max(a['max'][i] for a in acc) for i in range(3)]
        # glTF Y-up (x, y, z) -> Blender Z-up (x, -z, y), metres -> cm
        out[node['name']] = ([lo[0] * 100, -hi[2] * 100, lo[1] * 100], [hi[0] * 100, -lo[2] * 100, hi[1] * 100])
    return out


def fit_matrix(src_min, src_max, box):
    """3x4 affine rows that turn a mesh onto its motion axes and scale/move it onto `box` (same units in and out).

    Axes are matched by size (longest source axis -> longest box axis), so an upright import lies down the
    way the recorded box does. If that matching would mirror the mesh, the smallest axis is flipped as well,
    so the determinant stays positive (no inside-out normals).
    """
    lo, hi = box['min_cm'], box['max_cm']
    src = sorted(range(3), key=lambda i: src_max[i] - src_min[i])
    dst = sorted(range(3), key=lambda j: hi[j] - lo[j])
    perm = [0, 0, 0]
    for i, j in zip(src, dst):
        perm[j] = i
    odd = sum(1 for a in range(3) for b in range(a + 1, 3) if perm[a] > perm[b]) % 2
    rows = []
    for j in range(3):
        i = perm[j]
        d = src_max[i] - src_min[i]
        s = (hi[j] - lo[j]) / d if d > 1e-9 else 1.0
        row = [0.0, 0.0, 0.0, 0.0]
        if odd and j == dst[0]:
            row[i], row[3] = -s, hi[j] + s * src_min[i]
        else:
            row[i], row[3] = s, lo[j] - s * src_min[i]
        rows.append(row)
    return rows


def palette(path=PALETTE_MD):
    """{name: (r, g, b)} in linear 0-1, read from the palette.md table."""
    with open(path, encoding='utf-8') as fh:
        found = re.findall(r'^\| (\w+) \| #([0-9A-Fa-f]{6}) \|', fh.read(), re.M)
    out = {}
    for name, hx in found:
        srgb = [int(hx[k:k + 2], 16) / 255 for k in (0, 2, 4)]
        out[name] = tuple(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in srgb)
    return out
