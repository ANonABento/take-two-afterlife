# Art pass review (Task 1)

Scope: `/Game/Maps/Heaven` (task 6), imported assets (task 8: SK_Angel, halo, wings, 6 hats, Heaven meshes), backyard kit pieces (Kenney City Suburban, Kenney Nature, KayKit Prototype Bits).

## Results by check

| Check | Finding | Status |
|---|---|---|
| Scale (UE cm) | Angel 1.32 m tall × 0.85 m wingspan × 0.5 m deep; Heaven set 33.7 × 9.1 × 34.0 m; hats 0.41 m tall row. Couch seat height matches angel sit pose. | OK |
| Toon / flat look | All kits route through `M_TTA_Toon` palette instances (`TTA.Art.AllToon`, `Scripts/swap_to_toon.py`). | OK |
| Outline thickness | Single post-process outline `M_TTA_PP_Outline`; no per-mesh inverted-hull shells, so width is uniform. | OK |
| Palette | 12 colours, written to `docs/art/palette.md`. | OK |
| Pivots | Heaven cloud/couch/screen pivots were at mesh centre → **fixed** to base centre (z=0). | Fixed |
| Collision | Simple box/convex collision on Heaven props on import; kit props use UE auto-convex. | OK |
| Tri budget | Heaven 12,236 tris, angel 2,288, hats 1,124 total. Far under any limit; package stays well under 1 GB. | OK |
| Missing refs | `models/build_heaven_map.py` imported `toon_spec` without putting `Scripts/` on `sys.path` → **fixed**. | Fixed |

## Fixes

### 1. Heaven prop pivots moved to base
Before: pivots at bounding-box centre, props float/sink when placed at z=0.
After: pivot at base centre; props sit on the floor plane.

![before](../../models/renders/review_pivots_before.png)
![after](../../models/renders/review_pivots_after.png)

### 2. `build_heaven_map.py` missing reference
Before: `check_art_refs.py` → `MISSING build_heaven_map.py: import toon_spec` (1 missing, rc=1).
After: added `sys.path.insert(0, <repo>/Scripts)` → `0 missing references` (rc=0).

## Verification
`python models/check_art_refs.py` → `checked 0 backyard props, 6 heaven slots / 0 missing references`. Run with `--packs <kit folder>` to also resolve the backyard kit FBXs (kits are not in the repo).
