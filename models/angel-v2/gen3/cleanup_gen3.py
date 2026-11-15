"""Gen 3 angel cleanup: raw Tripo model -> angel_gen3_clean.ply (input for rig_gen3.py).

Usage:  pip install trimesh fast-simplification requests numpy
        python cleanup_gen3.py <tripo_key_file> <out_dir>
Steps: download raw (task bd71ffe3-...) -> weld -> Y-up to Z-up -> scale to 1.0 m, feet at 0,
centred -> reduce to 60k (fast-simplification, NOT Blender Decimate: it opens holes) ->
drop scraps (<=10 faces) -> turn so wings sit at +X (face at -X) -> require 0 open edges.
Tested 2026-11-15: 426,300 -> 59,974 tris, 0 open edges, 0.432 x 0.887 x 1.0 m.
"""
import sys, os, io, numpy as np, requests, trimesh, fast_simplification

TASK = "bd71ffe3-52b3-427a-a6c0-9a6f139c8773"
TARGET = 60000
key = open(sys.argv[1]).read().strip()
out = sys.argv[2]; os.makedirs(out, exist_ok=True)

r = requests.get(f"https://api.tripo3d.ai/v2/openapi/task/{TASK}",
                 headers={"Authorization": f"Bearer {key}"}, timeout=30).json()
o = r["data"]["output"]
url = o.get("pbr_model") or o.get("model") or o.get("base_model")
raw = requests.get(url, timeout=90).content
print("RAW_BYTES", len(raw))
m = trimesh.load(io.BytesIO(raw), file_type="glb", force="mesh")
m = trimesh.Trimesh(m.vertices, m.faces, process=False)
m.merge_vertices(merge_tex=True, merge_norm=True)  # weld seam-split verts
print("RAW_TRIS", len(m.faces))

# Y-up -> Z-up
m.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
lo, hi = m.bounds
m.apply_scale(1.0 / (hi[2] - lo[2]))
lo, hi = m.bounds
m.apply_translation([-(lo[0] + hi[0]) / 2, -(lo[1] + hi[1]) / 2, -lo[2]])

v, f = fast_simplification.simplify(m.vertices.astype(np.float32), m.faces.astype(np.int32),
                                    target_reduction=1 - TARGET / len(m.faces))
m = trimesh.Trimesh(v, f, process=True)
m.update_faces(m.nondegenerate_faces()); m.update_faces(m.unique_faces())
m.remove_unreferenced_vertices()

parts = sorted(m.split(only_watertight=False), key=lambda p: len(p.faces), reverse=True)
kept = [p for p in parts if len(p.faces) > 10]
print("PARTS", [len(p.faces) for p in parts[:5]], "DROPPED", len(parts) - len(kept))
m = trimesh.util.concatenate(kept)

# wings = 2nd/3rd largest parts; they must end up at +X (back), face at -X
wx = np.mean([p.centroid[0] for p in kept[1:3]]) if len(kept) >= 3 else 0
if wx < 0:
    m.apply_transform(trimesh.transformations.rotation_matrix(np.pi, [0, 0, 1]))
    print("TURNED_180")

edges = trimesh.grouping.group_rows(m.edges_sorted, require_count=1)
print("TRIS", len(m.faces), "OPEN_EDGES", len(edges), "SIZE", np.round(m.extents, 3))
assert len(edges) == 0, "open edges after reduction"
m.export(os.path.join(out, "angel_gen3_clean.ply"))
print("SAVED")
