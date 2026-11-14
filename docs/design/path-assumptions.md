# Code that assumes a straight x-axis corridor

Checked 2026-11-14 for the themed-maps plan (paths that turn and climb). Each entry: where, what it assumes, what breaks on a turning path.

## Core
- `Core/include/tta/Placement.h` — `kCorridorLengthMeters = 60` (x in [0, 60]) and `kCorridorWidthMeters = 8` (y in [-4, 4]). One fixed box in world space; no notion of a path made of segments.
- `Core/include/tta/Placement.h` — `Placement::cellX` / `cellY` are world-space 0.5 m cells. A trap on a turned segment still snaps to the world grid, not the segment's own grid; no height (z) is stored.
- `Core/include/tta/Placement.h` — `kRotationSteps = 8` (45° steps). Planned turns are multiples of 15°, so trap yaw cannot follow a 15°/30° segment.
- `Core/src/Placement.cpp` `inCorridor()` — `0 <= x <= L && |y| <= W/2`. Used by `Layout::canPlace` (so by `placeTrap`, `placeCamera`, the ghost preview) and by `parseLayout` (JSON load). Every placement off that box is `OutOfBounds`; this is the main blocker.
- `Core/src/Placement.cpp` `parseLayout` — rebuilds positions from `cellX`/`cellY` and re-checks them with the same box.
- `Core/tests/PlacementTests.cpp`, `Core/tests/MatchFlowTests.cpp` — bounds tests hard-code x 0…60, y ±4 (e.g. `60.3` and `4.5` are out).
- `Core/include/tta/MatchFlow.h`, `RunRecorder.h`, `Scoring.h`, `TrapTimeline.h` — include `Placement.h` for the `Layout` type only; no corridor maths found in their `.cpp`.

## UE (Source/)
- `TTABuildPawn.cpp` — ghost snaps world `X`/`Y` cm to cells (`snapToCell(GhostCm.X / 100)`), yaw = `45 * RotationStep`; asks `canPlace` (corridor box) for the ghost colour.
- `TTAGameMode.cpp` (`placeTrap` / `placeCamera` from `ServerPlace`) — passes world X/Y straight to `Layout`.
- `TTAPlacementMarker.cpp` — spawns at `cellToMeters(CellX/CellY)` with a separate Z; no segment height.
- `TTAMatchCarry.h`, `TTATrapActor.h` — carry/use `Placement` cells as world positions.
- `Tests/TTAMapStartToGateTest.cpp` — start within 2 m of x = 0, gate within 2 m of x = L, start→gate ≥ 90 % of L measured on X, kill plane spans x 0…L and y ±W/2.

## Python
- `Scripts/maplib/validate.py` — floor gap check walks along x; dressing/marker checks use the x/y box; start→gate measured on X. Fine for Backyard, must move to segment frames in plan task 2.
- `Scripts/build_backyard_map.py` — places props at world x/y from `backyard_layout`; no change needed while layouts output world positions.

## Verdict
Core assumes the straight corridor: trap/camera placement is bounded by one x/y box with 45° yaw. Before turning maps (plan task 3) Core needs a change task: bounds from the map's build cells (list of allowed cells or segment boxes, sent with the seed) instead of `inCorridor`, and 15° rotation steps (or per-segment base yaw).
