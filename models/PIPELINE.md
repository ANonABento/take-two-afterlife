# angel-v2 asset pipeline — source of truth (read this first after any context reset)

## Tripo sources
- API key: `/workspace/keys/tripo_tsk.txt` (stage via run_python `files`; it lands in the cwd as `tripo_tsk.txt`, NOT at its /workspace path).
- Fetch fresh URLs: `GET https://api.tripo3d.ai/v2/openapi/task/<id>` with `Authorization: Bearer <key>` → `data.output.pbr_model` (hats) / `model`.
- Hat task IDs (also in `hats/tripo_tasks.json`):
  - halo 90cb24e5-1bf0-4a0a-b3cf-126c43c6ffe0
  - devil-horns 603699c8-80bf-428f-b134-bc6c98bb284c
  - propeller-beanie c83c1ce9-24d8-49c8-90e2-36cea30ad040
  - party-cone 6110b186-f571-4ba5-89d4-ed1b0d4fa631
  - viking-helmet 3094b39c-0249-4f69-a9e2-3e0e8f34183f
  - crown fee83877-531c-435f-8377-32e5ebe55fbe
  - bunny-ears 0cdc2fa6-f848-49e3-92b9-e30584b35491
  - traffic-cone cd47b502-2255-43f4-80d8-39f760d4172f
- Angel: gen b1cca466-a937-4baf-acc9-630237ad898a, gen2 9f7f6369-64a2-426d-9bf6-119a839e5414, rig 1743b0df-1985-4c51-ae2b-5388256da3f5 (`gen/task_*.json`). Angel gen ran with texture off → no raw colour.

## Why raws are never in the workspace
- Raw crown GLB = 43.9 MB; run_python can only save outputs ≤10 MB. Raws CANNOT be persisted.
- Rule: every build run re-downloads the raw (~1 s) at its start and does weld → simplify in the same run. Save only a reduced/welded intermediate (<10 MB).

## Hat workflow (v3)
1. Download raw, apply scale.
2. Cleanup on raw: merge by distance 1e-7 m, delete loose/tiny islands, dissolve degenerate, recalc normals, fill holes ≤ small sides only.
3. Open edges must be 0 here.
4. Reduce with `fast-simplification` (NOT Blender Decimate Collapse — it opens holes) to ≤3k tris (angel ≤10k).
5. Re-import in Blender, merge by distance 1e-7 again (fixes the 26 rounding gaps), assert open edges == 0.
6. Smart UV 66°, margin 0.003. Bake selected-to-active, cage extrusion 0.01, margin 16 px: base colour + normal map (1024 hats / 2048 angel).
7. Smooth by angle 30°. Task-6 fixes: rotation, width, origin at base.
8. Export .glb/.fbx/.blend; check script fails on any open edge.

## Status (2026-11-14)
- Crown v2 (`hats/crown/v2/`): user says better but not perfect; 26 open edges, no normal map.
- Crown v3, other 7 hats, angel: not started.

## Status update (2026-11-14, crown v3 run)
- run_python: do NOT use workspace=models/angel-v2 for builds (whole dir counts toward 10 MB → nothing saves). Stage files flat via `files`, write outputs to relative `models/angel-v2/...` paths.
- fast-simplification output had 11 duplicate faces (folded pairs → edges used 4×); Blender drops dupes on import → 23 open edges. Fix: drop BOTH copies of every duplicated face in trimesh before Blender. Result: 2,878 tris, 0 open edges, 1 non-manifold edge.
- PLY import stays Y-up but the glTF importer converts the raw to Z-up → low must be rotated +90° X before FIT, otherwise bake rays miss everything.
- Crown v3 (`hats/crown/v3/`): geometry clean, normal map bakes real detail (std 0.10). COLOUR BAKE STILL ALL BLACK (both DIFFUSE/COLOR and EMIT-from-base-texture tried; raw has image `Color_<task>` on Base Color). Unsolved — next step: check raw image actually loaded (pixel stats of the raw texture), its UV map, and that v2's working bake differs only in low-mesh source.
- No comparison sheet yet. Other 7 hats and angel not started.


## Status update (2026-11-14, crown v3 colour fixed)
- ROOT CAUSE of black colour bake: apt Blender 3.4.1 has no OpenImageDenoiser; with denoising on, every Cycles bake/render returns all-zero (even a constant red emission). Fix: `sc.cycles.use_denoising = False` (now in rebuild_hat_v3.py). Also explains the blank cycles_compare.png earlier.
- Workbench renders need `apt-get install libegl1 libgl1 libegl-mesa0` or Blender aborts (no libEGL).
- Saving: total outputs per run ≤10 MB → save .glb in a separate run (re-export from the saved .blend).
- Staging two files with the same basename fails → stage one, export the other to /tmp inside the run.
- Crown v3 DONE in hats/crown/v3/: 2,878 tris, 0 open edges, 1 UV map, colour + normal 1024 baked (colour mean matches raw), 0.331×0.341×0.272 m. compare_v3.png = raw | v2 | v3, clay + textured. Awaiting user review before batching 7 hats + angel.


## Status update (2026-11-14, triangle experiment)
- User: v3 crown "good enough to use"; asked to test higher tri counts without regenerating.
- hats/crown/tri_test/{10k,20k}/crown.glb: 9,960 / 19,972 tris, 0 open edges, colour+normal 1024 baked, same pipeline (rebuild_hat_v3.py works unchanged for any PLY). Both build in ~15 s each.
- Awaiting user pick of hat budget (3k / 10k / 20k), then batch the other 7 hats, then angel.


## Status update (2026-11-14, hat batch at 20k)
- User decision: Windows PC, ~4 angels on screen → hats 20k (no LOD), angel 25–30k (+10k LOD).
- `hats/build_hat_v3.py <name> 20000 /tmp/out` = download raw → weld → drop ≤50-tri islands → fast-simplify → drop duplicate faces → rebuild_hat_v3.py. ~15 s per hat. Task IDs are under `["tasks"]` in tripo_tasks.json.
- ALL 8 HATS DONE in `hats/<name>/<name>.{glb,fbx,blend}` (overwrote task-9 files): 19,906–19,996 tris, 0 open edges, 1 UV map, colour + normal 1024 baked (colour std 0.26–0.47, none flat). party-cone 2 / viking 1 non-manifold edges (pinch points, not holes).
- Save cap: glb+fbx+blend for one hat ≈ 8–10 MB → export the .glb from the saved .blend in a second run when the first run is over the cap.
- Stale now: hats/contact.png, hat_fit.json heights, tripo_dec.glb, render_34.png, crown/v2, v3, tri_test (scratch).
- Angel: raw = gen2 9f7f6369 (multiview_to_model, output `model`), 1,981,309 faces, 0.43×0.98×0.79 extents (not yet aligned to final), 6 parts, 1,028 open edges after weld → needs hole fill or voxel remesh before simplify. Not rebuilt yet.
- Angel plan: weld+fill raw → simplify to 30k → align to final/angel.blend → transfer weights + material slots (Body/Outfit/Face) from current Angel via Data Transfer → re-UV (Face island 0–1) → bake colours from current Angel → keep head_hat bone → test pose render.


## Status update (2026-11-14, angel v4 step 1)
- User: angel is very important; raise tri count if needed. Target ~30k (+10k LOD later).
- Weld with trimesh merge_vertices(merge_tex=True, merge_norm=True, digits_vertex=7) on raw gen2 → body only 62 open edges (the earlier 1,028 was from welding without merge_tex). 6 parts = body 1.77M, two closed parts 98k (0.09×0.19×0.15) and 111k (0.27×0.02×0.27, likely halo), 3 single-face junk.
- fill_holes → fast-simplify per part (proportional to 30k, min 2k) → drop duplicate faces: total 30,760 tris, 0 open edges, 13 non-manifold edges. Saved `final/v4/angel_low.ply` (still raw orientation/scale, 0.43×0.98×0.79).
- Next: Blender — align to final/angel.blend, Data Transfer weights + slots, UV (Face island 0–1), bake colours from current Angel, keep head_hat, test pose render.


## Status update (2026-11-14, angel v4 alignment attempt)
- Current final/angel.blend: Angel mesh 9,176 tris, 0.861×0.466×1.0 m Z-up, origin at feet, slots Body/Outfit/Face, 18 vertex groups, rig AngelRig 19 bones incl. head_hat.
- angel_low.ply → Y-up→Z-up, scale to 1.0 m tall, tried 4 yaw rotations + ICP vs current verts. Best yaw 90° (cost 0.104) but 270° almost equal (0.105) → front/back NOT resolved. Result 0.805×0.438×1.012 m. Mean gap to old mesh 4.9 cm, p95 10.4 cm → NOT good enough for weight transfer. Saved final/v4/angel_aligned.ply (unverified).
- Next: render old vs aligned overlay (front + side) to see if mismatch is facing, wings, or a different shape; decide facing from the Face slot centre (old Face polys = front); then ICP on body only (exclude wings/halo).


## Status update (2026-11-14, angel source decision)
- User: use the SECOND generation (gen2 9f7f6369, the one without the face) for the angel.
- angel_low.ply / angel_aligned.ply were ALREADY built from gen2. So the shape mismatch vs final/angel.blend is not a wrong-raw mistake: the current rigged angel came from a different source (gen1 or the rig task output), so Data Transfer of weights from it is invalid.
- Fitting notes: old final is Y-up in glb, new raw Z-up; with axes matched, body-only gap 5–6 cm mean for all 4 yaws → different shape.
- Decision: re-rig gen2 mesh directly. Place the 19 AngelRig bones (incl. head_hat) on gen2 body landmarks, automatic weights, test-pose render for user review. Gen2 has no face → Face slot/face UV island needs a decision (decal on head front, or drop).


## Status update (2026-11-14, angel v5)
- User: v4 looked better than old but too low-poly; remove halo; v4 was misoriented.
- final/v5/angel_low.ply: gen2 raw, weld (merge_tex) → dropped halo part (0.27×0.024×0.27) and 3 junk faces → fill_holes → fast-simplify to 60k total → drop duplicate faces → Y-up→Z-up, feet at z=0, scaled 1.0 m tall. 59,900 tris, 0 open edges, extents 0.466×0.862×1.0 (needs +90° Z to face like old final; applied in preview only).
- preview_v5.png: 4 views (0/90/180/45°) Workbench clay. Awaiting user review before rigging.
- Run note: staged files land flat in cwd (angel_low.ply, not its workspace path).


## Status update (2026-11-14, angel GEN3)
- Gen2 abandoned: wings generated on the CHEST. Likely cause: gen2 put the LEFT side image in the RIGHT slot (mirrors depth → front/back swap).
- GEN3 task id: **bd71ffe3-52b3-427a-a6c0-9a6f139c8773** (multiview_to_model, success). Request + status in `gen3/task_gen3.json`, `gen3/task_gen3_status.json`. Output key `pbr_model`.
- Inputs: gen3/front_nohalo.png (front), left_nohalo.png (LEFT slot), back_nohalo.png (back), right empty. Halo painted out (rows 150–212).
- Settings: texture on, pbr on, texture_quality detailed, auto_size on, generate_parts off.
- Next: download raw, render front/side/back clay + textured, user confirms wings on back BEFORE any cleanup. Then weld → simplify ~60k → head smooth 15 passes → rig (front = face side, verify!) → slots/bakes/LOD/export.
