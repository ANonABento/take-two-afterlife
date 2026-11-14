# Traps — design target

Status: design doc, no code changes. Based on repo HEAD 9a72b30 (Nov 14 2026) and the sheet `models/angel-v2/ref/sheet-traps.png`.

The sheet has **7 traps**; the code has **6** (`enum class TrapType { Hammer, Cannon, Spinner, PusherWall, Trapdoor, Banana }` in `Core/include/tta/Placement.h`). This doc describes the sheet's 7 as the target and names, for each, the code `TrapType` it replaces or extends. A later code task owns renaming `TrapType`, the catalogue (`Core/src/Placement.cpp`), `TrapTimeline` and the `models/SM_Trap_*.fbx` names.

Grid: 0.5 m cells, 45° rotation steps (`kGridMeters = 0.5`, `kRotationSteps = 8`), corridor 60 × 8 m, budget 100 (`kBudget`).

**Footprints below are proposals.** Today a `Placement` is one cell (`cellX`, `cellY`) with no footprint, and there is no overlap check — two traps can sit on the same cell. Enforcing these footprints needs an occupancy check in `Layout::canPlace`.

| Trap | Code type | Footprint (cells) | Cost | House | Sky Construction | Pirate Dungeon |
|---|---|---|---|---|---|---|
| Boxing glove | PusherWall (reskin) | 1 × 2 | 15 | yes | yes | yes |
| Windmill sweeper | Spinner (reskin) | 2 × 2 + sweep | 20 | yes | yes | no |
| Cannon | Cannon (same) | 1 × 2 | 25 | no | yes | yes |
| Giant mousetrap | Hammer (reskin) | 2 × 3 | 20 | yes | no | yes |
| Foam pendulum | new | 1 × 1 + 4 × 1 lane | 20 | yes | yes | yes |
| Bounce plate | Trapdoor (reskin) | 2 × 2 | 15 | yes | yes | no |
| Banana launcher | Banana (extends) | 1 × 1 + landing zone | 5 | yes | no | yes |

## Traps

### Boxing glove
- **Footprint:** 1 × 2 cells (0.5 × 1.0 m) base, pivot at the rear cell centre; punch reach 1 m (2 cells) in front, shown as a separate ghost zone. Proposal.
- **Setting 1:** force, 1–5, default 3 — how far the runner is knocked back.
- **Setting 2:** cycleTime, 1–5, default 3 — seconds between punches.
- **Cost:** 15
- **Hit:** glove face hits the runner along the trap's facing direction; fixed loop every cycleTime s (deterministic); runner ragdolls and slides back/sideways by force.
- **Maps:** House — yes, spring-loaded toy fits kids' rooms and hallways. Sky Construction — yes, can punch runners off girder edges. Pirate Dungeon — yes, reads as a ship's ram/gag in stone corridors.
- **Code type:** PusherWall (rename/reskin; same push motion).

### Windmill sweeper
- **Footprint:** 2 × 2 cells (1 × 1 m) base, pivot at the centre corner of the 4 cells; sweep radius = armLength (1–3 → ~1–3 m). Proposal.
- **Setting 1:** speed, 1–5, default 3 — rotations per period.
- **Setting 2:** armLength, 1–3, default 2 — sweep radius.
- **Cost:** 20
- **Hit:** blade arms sweep at ankle/shin height, tangent to rotation; continuous fixed rotation; runner trips and ragdolls sideways.
- **Maps:** House — yes, fan/toy windmill in the living room. Sky Construction — yes, a crane-style sweeper on a wide deck. Pirate Dungeon — no, no windmill theme; a capstan reskin would be needed.
- **Code type:** Spinner (rename/reskin).

### Cannon
- **Footprint:** 1 × 2 cells (0.5 × 1.0 m), pivot at the breech cell; ball travels along facing. Proposal.
- **Setting 1:** fireRate, 1–5, default 3 — shots per period.
- **Setting 2:** ballSize, 1–3, default 2 — ball radius and hit width.
- **Cost:** 25
- **Hit:** projectile `SM_Trap_CannonBall` travels straight along facing; fires on a fixed timer; runner ragdolls in the ball's direction.
- **Maps:** House — no, a cannon is off-theme indoors (could be a toy-cannon reskin later). Sky Construction — yes, long open sightlines down beams. Pirate Dungeon — yes, the signature pirate trap.
- **Code type:** Cannon (same).

### Giant mousetrap
- **Footprint:** 2 × 3 cells (1.0 × 1.5 m), pivot at the hinge-bar centre. Proposal.
- **Setting 1:** swingSpeed, 1–5, default 3 — how fast the bar snaps. Changes from code meaning: same range, but drives a one-shot snap instead of a swing.
- **Setting 2:** direction, 0–1, default 0 — which end the bar snaps toward.
- **Cost:** 20
- **Hit:** snap bar slams down onto the plate; snaps then resets on a fixed loop (not triggered, keeping determinism); runner on the plate is flattened then ragdolls forward.
- **Maps:** House — yes, a giant mousetrap on a kitchen floor is the joke. Sky Construction — no, no fit with steel and falls. Pirate Dungeon — yes, works as a dungeon rat trap.
- **Code type:** Hammer (reskin; swing becomes one-shot snap on a loop).

### Foam pendulum
- **Footprint:** 1 × 1 cell (0.5 × 0.5 m) mount, pivot at the mount; 4 × 1 cell (2.0 × 0.5 m) swing lane across the facing. Proposal.
- **Setting 1:** swingSpeed, 1–5, default 3 — swing period. Changes from code: new type; borrows Hammer's range.
- **Setting 2:** armLength, 1–3, default 2 — arc width (maps to lane length). Changes from code: new type; borrows Spinner's armLength range.
- **Cost:** 20
- **Hit:** soft foam bob swings across the lane, perpendicular to the corridor; continuous pendulum loop; runner is knocked sideways into ragdoll.
- **Maps:** House — yes, hung from a doorway or ceiling. Sky Construction — yes, a swinging load on a crane hook. Pirate Dungeon — yes, a swinging log/anchor on ropes.
- **Code type:** new (needs a new `TrapType`; Hammer is taken by the mousetrap).

### Bounce plate
- **Footprint:** 2 × 2 cells (1.0 × 1.0 m), pivot at the centre. Proposal.
- **Setting 1:** openDelay, 0–5, default 1 — seconds before the plate fires each loop.
- **Setting 2:** openDuration, 1–5, default 2 — how long the plate stays up (also scales launch height). Changes from code meaning: duration of fling, not of an open hole.
- **Cost:** 15
- **Hit:** the plate kicks up under the runner's feet, launching them upward; timed loop; runner ragdolls in the air and lands.
- **Maps:** House — yes, a trampoline/spring floor panel. Sky Construction — yes, launching runners off the edge is the best gag there. Pirate Dungeon — no, stone floors suit trapdoors more than springs.
- **Code type:** Trapdoor (reskin; flings up instead of dropping).

### Banana launcher
- **Footprint:** 1 × 1 cell (0.5 × 0.5 m) launcher, pivot at centre; landing zone in front sized by slideDistance. Proposal.
- **Setting 1:** slideDistance, 1–5, default 3 — how far a runner slides after slipping (and how far peels land).
- **Setting 2:** count, 1–3, default 1 — peels launched per cycle.
- **Cost:** 5
- **Hit:** peels land in the zone; a runner who steps on one slips and slides forward along their motion; peels launch on a fixed loop (code today: static, `trapPeriodMs` 0 — this needs a period); runner ragdolls on their back.
- **Maps:** House — yes, a kitchen gag. Sky Construction — no, slips into falls feel unfair with no read time. Pirate Dungeon — yes, wet stone and galley scraps fit.
- **Code type:** Banana (extends: launches peels instead of placing static ones).

## Cameras

Cameras are **player-placed**: free (cost 0), cap 12 per owner, 4 types — Fixed, Pan, Follow, TrapCam.
- `Core/include/tta/Placement.h` — `enum class CameraType { Fixed, Pan, Follow, TrapCam }`, `Layout::placeCamera(owner, CameraType, x, y, deg)`, `cameraCount(owner, type)`, `kCameraCap = 12`.
- `Source/TakeTwoAfterlife/Private/TTABuildPawn.cpp:50` — calls `placeCamera` or `placeTrap` based on the selected item.
- `Source/TakeTwoAfterlife/Private/TTAGameMode.cpp:316-317` — same, server-side.
- `Source/TakeTwoAfterlife/Private/TTAPlacementMarker.cpp:22-31` — build items are traps + 4 camera types, labelled "Camera: %s (free)"; `:91` raises cameras to `CameraHeightCm`.
- `Source/TakeTwoAfterlife/Private/TTABuildHUD.cpp:23-25` — HUD shows "CAMERAS n / 12".

The **Heaven Cam is not placed**: it is a system camera (`plan.md` line 170; `Source/TakeTwoAfterlife/Private/TTARunDirector.cpp:30,121`; `TTAEditorWidget.cpp:183` falls back to it when no cameras are placed).

## Proposal: blocker

- **Crate:** 1 × 1 cell (0.5 × 0.5 m). **Plank:** 1 × 4 cells (0.5 × 2.0 m).
- Static, no settings, no loop. Cost 0–5 (suggest crate 0, plank 5).
- Blocks runners and line of sight (cameras and cannon shots).
- Maps: suits all three — House (boxes/furniture), Sky Construction (crates/planks on girders), Pirate Dungeon (barrels/planks).
- Needs a new item kind (neither `TrapType` nor `CameraType`) and its own cap so blockers can't wall off the corridor.

## Sources
- `plan.md`
- `Core/include/tta/Placement.h`
- `Core/src/Placement.cpp`
- `Core/include/tta/TrapTimeline.h`
- `Source/TakeTwoAfterlife/Private/TTATrapActor.cpp`
- `Source/TakeTwoAfterlife/Private/TTABuildPawn.cpp`
- `Source/TakeTwoAfterlife/Private/TTAGameMode.cpp`
- `Source/TakeTwoAfterlife/Private/TTAPlacementMarker.cpp`
- `Source/TakeTwoAfterlife/Private/TTABuildHUD.cpp`
- `Source/TakeTwoAfterlife/Private/TTARunDirector.cpp`
- `Source/TakeTwoAfterlife/Private/TTAEditorWidget.cpp`
- `models/build_traps_cams.py`
- `models/angel-v2/ref/sheet-traps.png`
