# One hat end-to-end: download raw -> weld/clean -> fast-simplify -> dedupe faces -> PLY -> Blender bake/export.
# python build_hat_v3.py <name> <target_tris> <out_dir>   (needs tripo_tsk.txt, tripo_tasks.json, rebuild_hat_v3.py in cwd)
import sys, json, os, subprocess, requests, numpy as np, trimesh, fast_simplification
N, T, OUT = sys.argv[1], int(sys.argv[2]), sys.argv[3]
os.makedirs(OUT, exist_ok=True)
key = open("tripo_tsk.txt").read().strip()
tid = json.load(open("tripo_tasks.json"))["tasks"][N]
d = requests.get(f"https://api.tripo3d.ai/v2/openapi/task/{tid}", headers={"Authorization": f"Bearer {key}"}).json()["data"]["output"]
url = d.get("pbr_model") or d.get("model")
raw = f"/tmp/{N}_raw.glb"; open(raw, "wb").write(requests.get(url).content)
m = trimesh.load(raw, force="mesh", process=False)
m.merge_vertices(merge_tex=True, merge_norm=True, digits_vertex=7)
parts = m.split(only_watertight=False)
m = trimesh.util.concatenate([p for p in parts if len(p.faces) > 50])
m.remove_unreferenced_vertices(); m.update_faces(m.nondegenerate_faces())
print("RAW", N, "faces", len(m.faces), "parts", len(parts))
v, f = fast_simplification.simplify(m.vertices.astype(np.float32), m.faces.astype(np.int64), target_reduction=1 - T / len(m.faces))
key_ = np.sort(f, axis=1); u, inv, cnt = np.unique(key_, axis=0, return_inverse=True, return_counts=True)
f = f[cnt[inv.ravel()] == 1]
lo = trimesh.Trimesh(v, f, process=False); lo.remove_unreferenced_vertices()
eu = trimesh.grouping.group_rows(lo.edges_sorted, require_count=1)
print("LOW", N, "tris", len(lo.faces), "open", len(eu))
ply = f"/tmp/{N}_low.ply"; lo.export(ply)
r = subprocess.run(["blender", "-b", "--factory-startup", "--python-exit-code", "1", "--python", "rebuild_hat_v3.py", "--", raw, ply, N, OUT], capture_output=True, text=True)
print("\n".join(l for l in r.stdout.splitlines() if l.split(" ")[0] in ("LOW", "HAT", "PIX", "DONE", "RAWMATS")))
print("RC", r.returncode, r.stderr[-800:] if r.returncode else "")
