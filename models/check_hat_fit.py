# Count hat vertices inside the angel body mesh (world space, after bone parenting). Exit 1 if any hat > 0.
# python check_hat_fit.py <hat_world_verts.npz> <hat_fit.json>
import sys, json, numpy as np, trimesh
d = np.load(sys.argv[1]); fit = {k: v for k, v in json.load(open(sys.argv[2])).items() if not k.startswith("_")}
body = trimesh.Trimesh(d["body"], d["body_faces"], process=False); body.merge_vertices()
print("body tris", len(body.faces), "watertight", body.is_watertight)
assert body.is_watertight, "body must be closed for an inside test"
bad = 0
for n in fit:
    k = int(body.contains(d[n]).sum()); bad += k > 0
    print(f"{n:18s} verts {len(d[n]):6d}  inside {k}")
print("ALL 8 CLEAR" if bad == 0 and len(fit) == 8 else f"FAIL {bad} hats / {len(fit)} listed"); sys.exit(1 if bad or len(fit) != 8 else 0)
