"""Engine-free trap size/pivot rules shared by verify_traps_cams.py (Blender) and the unittest suite.

Boxes live in trap_bounds.json (cm, Blender Z-up, relative to the pivot). Because the box is stored
relative to the pivot, a mesh whose pivot drifts off the plan.md convention also moves outside its box.
"""
import json, os, struct

BOUNDS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'trap_bounds.json')
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
