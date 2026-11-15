# Hat fit — handoff (Nov 15, 2026)

Status: partial. Good enough to keep moving on the game; final tuning should be done by hand in a hat-fit editor (see below).

## Facts that took a while to learn
- The gen-3 angel faces **-X** (confirmed by the user from a render). +X is the back of her head.
- `head_hat` bone head is at z = 1.00 (top of the 1.0 m angel).
- The Tripo hat .glbs are single meshes with hundreds of loose, unclosed pieces, so inside/outside tests are unreliable unless you voxel-remesh them first (3 mm) — see models/hatfit/step1_remesh_facing.py.
- Beanie, viking, ears and horns are worn: the head goes inside them, so overlap is allowed. Crown, cones and halo sit on top: 0 hat verts inside the head.
- The bunny/horn headbands are pinched loops; scaling alone can't wrap a 0.55 m head. fit_v9.py bends the band onto the skull.

## Current fits
- beanie: scale 2.0, 10 cm lower, back tilted down 14°, head showing 1.7% ✅
- viking: scale 2.2, turned 180°, rim z 0.76, head showing 0.2% ✅
- bunny-ears: band bent to skull, 0 into head, gap 24 mm ⚠️, mirror 29 mm ⚠️
- devil-horns: band bent to skull, 0 into head, gap 4.7 mm, mirror 8 mm ⚠️
- crown / party-cone / traffic-cone / halo: from the v3 fit (hat_fit.json at workspace root); traffic cone rim gap 4.4 mm vs 4 mm target.

## Files
- hatfit_v9/fit_v9.py + fit_v9.json: beanie, viking, ears, horns
- fit_v3.py + hat_fit.json: crown, cones, halo
- hatfit_v9/hat_fit_v9_sheet.jpg: latest preview
- Fitted .glb/.blend are not saved (over 10 MB); rerun the scripts to rebuild them.

## Recommended next step (user's idea)
Build a small hat-fit editor: load the angel + one hat on head_hat, sliders/gizmo for scale, rotation and offset, and an Export button that writes hat_fit.json in the same schema. Use the numbers above as starting values. This replaces automated fitting for final polish.
