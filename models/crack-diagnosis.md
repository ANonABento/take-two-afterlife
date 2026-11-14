# Crack diagnosis (task 9)

**Cause: flat shading (angel) and holes from triangle reduction (hats). Not torn texture seams** — neither model had a UV colour texture to tear.

- **Angel:** Tripo job ran with `texture: False`, so the raw has no colour. The old final was fully flat-shaded: 27,452 verts for 9,176 tris (3 per tri, 0 smooth faces), and the wings were separate objects. Every face edge read as a crack.
- **Hats:** `tripo_dec.glb` was decimated on vertex colour, which opened the surface: 1,262 open edges on the crown, 907 on the halo.

**Fix:** re-download raw textured hats (~1.4M tris), reduce to ≤3k, Smart UV Project, bake raw colour (Cycles selected-to-active, 1024 px), Smooth by Angle 30°. Angel rebuilt from its .blend (no raw colour), wings joined, slot colours baked to 2048 px, face island reset to 0–1, Body/Outfit/Face + head_hat kept, smooth 30°.

**Check (`check_task9.py`):** every model has 1 UV map + baked image; angel 9,176 tris, 3 slots; hats 2,898–2,899 tris. ALL OK.

See `crack-diagnosis.png`: left = old, right = rebuilt; top angel, bottom crown; shaded (clay) and coloured.
