# Angel v2 / task 9 retrospective (Nov 14, 2026)

Purpose: review later to improve Waymond (agent) and Choomfie (pipeline/library).

## Outcome
- Hats: 8 rebuilt at ~20k tris, 0 open edges, baked colour + normal 1024. Build: hats/build_hat_v3.py.
- Angel: gen 1/gen 2 abandoned. Gen 3 (Tripo task bd71ffe3-52b3-427a-a6c0-9a6f139c8773) approved by user on sight. Cleanup/rig pending.

## Why gen 3 is better (what changed)
1. Multiview slots fixed. Gen 2 put the LEFT side image in the RIGHT slot -> depth mirrored -> wings generated on the chest. Gen 3: front=front, left=left side, back=back, right empty.
2. Halo painted out of all 3 inputs (rows ~150-212 filled with background grey).
3. texture=true (gen 2 had texture=false -> grey angel all day), texture_quality=detailed, pbr on.
4. generate_parts=false -> one piece (gen 2 had 37 pieces incl. a stray third wing).
5. auto_size on (model came out 0.2 m; scale to 1.0 m in cleanup).

## Root causes of the day's problems
- Cracks: Blender Decimate (Collapse) opens holes on Tripo meshes; Tripo raw is split on UV seams. Fix: weld raw at 1e-7 m -> fast-simplification -> drop duplicate/folded face pairs -> re-weld in Blender -> require 0 open edges.
- Black bakes / blank renders: denoiser missing in sandbox Blender; must disable denoising. (Was already in lessons; not applied.)
- Silent Blender crashes on render: needs libegl1 libgl1 (and apt-get update).
- Outputs dropped: run_python save cap is 10 MB for the WHOLE staged workspace dir, not per file. Stage the smallest folder only.
- Raw models (17-44 MB) can never be saved; re-download each run from task ID (IDs in hats/tripo_tasks.json, gen/task_*.json, gen3/task_gen3.json; key keys/tripo_tsk.txt).
- Y-up (glTF) vs Z-up (Blender/PLY) mix-ups caused misalignment and a lying-down angel.
- Front/back guessed from geometry (0.229 vs 0.211, too close) -> wrong. Determine front from inputs/face, never from a near-tie.

## Waymond process failures (fix these)
- Claimed "done" from pixel stats without seeing images. Waymond cannot view workspace PNGs; say so up front and ask the user to look, every time.
- Guessed input files by name and presented them as fact (gen 2 inputs). Only state provenance from records.
- Promised steps (projection of user circles, ear mask protection, Blender render) then silently did something else. State deviations before the result, not after.
- Iterated automated head reshaping 9 times; each made new artifacts. Stop after 2 failures and propose a manual sculpt or regeneration.
- Repeated known mistakes (staging big dir, denoiser) despite lessons. Read lessons/3d.md before every run, not just at start.
- Linked scratch files, which get committed to the flat models/ folder. Link only final assets + scripts.
- Best fix came from fixing the INPUT (regenerate correctly) rather than patching OUTPUT. Check generation inputs/settings first when a generated mesh is structurally wrong.

## User feedback that worked
- User viewing images and circling problems was the real QA loop.
- "Cut and isolate the leftovers" was a good strategy for imperfect selections.
- Raising tri budget (hats 20k, angel ~60k for Windows, 4 angels) was the right call.

## Remaining angel pipeline (gen 3)
weld -> scale to 1.0 m (feet at 0, face -Y in Tripo frame; confirm) -> reduce ~60k -> 0 open edges -> 15-pass head smooth (vol-preserving, above neck only) -> rig 20 bones incl head_hat + 2 wing bones -> Body/Outfit/Face slots, face UV 0-1 -> bake colour+normal 2048 from raw -> 10k LOD -> export glb/fbx/blend to final/ -> re-render front/side/back + hats/contact.png -> check script (open edges, UVs, textures, 3 slots).
Then: one reply linking only final assets; git rm scratch files listed in chat.
