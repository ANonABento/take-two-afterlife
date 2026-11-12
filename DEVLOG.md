# Take Two Afterlife — Dev Log

Built for the Epic AI MegaJam 2026 (theme: Director's Cut). Each entry: the prompt, what the agent did, what failed, what was fixed by hand.

## Task 4 — Core scaffold + state machine
- **Prompt:** engine-free C++17 `Core/` with CMake + doctest, root `make test`, match state machine (Lobby → Build → Run → Edit → Premiere → next round or Podium) with rounds/timers from settings JSON.
- **Agent did:** added `Core/` (library + doctest tests), vendored doctest 2.4.11, a small flat-JSON settings parser, the state machine, and the `Makefile`.
- **Failed:** first build — doctest can't print `enum class Phase`; fixed with a `StringMaker<Phase>` in the tests. The sandbox also needed `pip install cmake`.
- **Fixed by hand:** nothing yet.

## Task 5 — Placement + budget model
- **Prompt:** trap catalogue, 0.5 m snap, 45° rotation, corridor bounds, budget 100, camera cap 12, hidden flag, JSON save/load.
- **Agent did:** added `Placement.h/.cpp` and doctest cases for snap, bounds, over-budget, camera cap, settings, hiding and JSON round-trip.
- **Failed:** two bounds tests used points that snap back inside the corridor (60.2 → 60.0); moved them further out.
- **Fixed by hand:** nothing yet.

## Task 6 — Deterministic trap timelines
- **Prompt:** pure `pose(trapType, settings, t)` for all 6 traps; `make test` with determinism and period tests per trap.
- **Agent did:** added `TrapTimeline.h/.cpp` (integer-ms phase, no randomness) and doctest cases covering every setting combination.
- **Failed:** nothing.
- **Fixed by hand:** nothing yet.

## Task 7 — Scoring
- **Prompt:** gate +1, trap wipeout = cost (min 1) to owner, all/none-finish voids trap points, round and match totals.
- **Agent did:** added `Scoring.h/.cpp` and doctest cases for every rule, including all-finish, none-finish, own-trap, unknown players and match totals/podium.
- **Failed:** nothing.
- **Fixed by hand:** nothing yet.

## Task 8 — Replay recorder
- **Prompt:** multi-player sampler (root + 15 bones at 20 Hz), hit/wipeout/smite events with trap id and owner, binary serialize, interpolated lookup; `make test` incl. 3-min 4-player size < 8 MB and interpolation error < 1 cm.
- **Agent did:** added `Replay.h/.cpp` (float32 positions, int16 rotations, Catmull-Rom position lookup, nlerp rotations) and doctest cases for size, interpolation error, round-trip, clamping and bad input.
- **Failed:** first build had 3.4 cm error in the first/last segment (edge sample repeated as the Catmull-Rom neighbour); fixed by extrapolating it. First check ran without cmake installed.
- **Fixed by hand:** nothing yet.

## Task 9 — Edit list + auto-edit
- **Prompt:** clip(camera, range) with trim, reorder, multi-angle, slow-mo, zoom, overlays, 30 s cap, auto-edit from hit events; `make test` per op, cap and auto-edit on a fixture replay.
- **Agent did:** added `EditList.h/.cpp` (copy-validate-commit ops, auto-edit from replay events) and doctest cases for each op, the cap and auto-edit on fixture replays.
- **Failed:** first check ran without cmake in the sandbox (false PASS); reran with cmake installed.
- **Fixed by hand:** nothing yet.

## Task 10 — Ratings
- **Prompt:** Funny→editor and Evil→builder 1–5, self-rating rejected, averages feed match score; `make test` incl. self-rating rejection and aggregation.
- **Agent did:** added `Ratings.h/.cpp` (`RatingBook`) and doctest cases for self-rating, bad input, per-player aggregation and feeding `scoreRound`.
- **Failed:** nothing.
- **Fixed by hand:** nothing yet.

## Task 11 — UE project + Core module + package smoke build
- **Prompt:** UE5 C++ project compiling Core/ as a module, empty Title map, Win64 package profile, zip script `TeamName_TakeTwoAfterlife` with < 1 GB check; `make test`, plus TTA.Core.Smoke and packaged zip checked on Kevin's machine.
- **Agent did:** added uproject, targets, `TTACore` + game module, `TTA.Core.Smoke`, Config ini files, package/zip/title-map scripts and Python tests for the zip script.
- **Failed:** UE build not runnable in the sandbox; untested until Kevin builds.
- **Fixed by hand:** nothing yet.

## Task 14 — Ragdoll pawn + impact frame
- **Prompt:** walk/jump/dive, ragdoll on hit, victim-only 0.12 s freeze with M_TTA_Toon flash, shake, SFX hook, respawn at start, wings off in human form; Automation TTA.Pawn.Ragdoll.
- **Agent did:** added engine-free `PawnLife` timing + doctest cases, `ATTARunnerPawn`, `TTA.Pawn.Ragdoll`, input mappings.
- **Failed:** sandbox containers don't keep files; first pass lost uncommitted work and was redone.
- **Fixed by hand:** nothing yet.

## Task 15 — EOS host/join by code
- **Prompt:** Epic login, host lobby shows a code, join by code, listen-server travel, direct-IP fallback, phase replication (req 9); Automation TTA.Net.PhaseReplication + a 2-PC code join on Kevin's machine.
- **Agent did:** added Core `JoinCode` (+ doctest), `UTTASessionSubsystem`, `ATTAGameMode`/`ATTAGameState` with replicated phase, EOS config and plugins, `TTA.Net.PhaseReplication`.
- **Failed:** sandbox containers don't keep files between runs, so the edit was redone as one script.
- **Fixed by hand:** nothing yet.

## Task 16 — Traps in world + recording + smite
- **Prompt:** trap actors driven by Core pose(), settings applied, server-side hit events into the Core replay at 20 Hz, smite on timeout with Heaven Cam; Automation TTA.Traps.Deterministic + TTA.Traps.HitRecorded.
- **Agent did:** added Core `RunRecorder` (+ doctest), `ATTATrapActor`, `ATTARunDirector`, both Automation tests.
- **Failed:** sandbox lacked cmake so the first check skipped Core tests; installed it and reran. UE code untested until Kevin builds.
- **Fixed by hand:** nothing yet.

## Task 17 — Backyard map + gate
- **Prompt:** themed ~60×8 m corridor with set dressing and a readable route from the assets-ue5.md shortlist, all materials on M_TTA_Toon, start, finish gate, kill plane, CREDITS entries; Automation TTA.Map.StartToGate + TTA.Art.AllToon.
- **Agent did:** engine-free `Scripts/backyard_layout.py` (zones, 80 props, start/gate/kill plane, validation, CREDITS check) + Python tests; editor script `Scripts/build_backyard_map.py`; `ATTAFinishGate`, `ATTAKillPlane`, `ATTARunDirector::HasFinished`; `TTA.Map.StartToGate`; Backyard added to `TTA.Art.AllToon`; CREDITS rows.
- **Failed:** Kenney packs download through a JS button, so their mesh file names could not be checked from the sandbox (KayKit names were checked on GitHub). UE code untested until Kevin builds.
- **Fixed by hand:** nothing yet.

## Task 18 — Build mode
- **Prompt:** free-fly build camera, ghost preview, snap/rotate, 2-setting panel, budget UI, camera placement by type, others' placements hidden until Run, replicated; Automation TTA.Build.PlaceReplicate.
- **Agent did:** Core `Layout::canPlace` (+ doctest), `ATTAPlacementMarker` (owner-only relevancy until Run), `ATTABuildPawn`, `ATTABuildHUD`, build requests on `ATTAGameMode`, input mappings, `TTA.Build.PlaceReplicate`.
- **Failed:** nothing in the sandbox checks; UE code untested until Kevin builds.
- **Fixed by hand:** nothing yet.

## Task 19 — Run phase
- **Prompt:** everyone races together, handheld camera, timer, respawn at start, scoring via Core, proximity voice; Automation TTA.Run.ScoreRound.
- **Agent did:** Core `RunRecorder::roundInput` (+ doctest), Run entry/exit + scoring in `ATTAGameMode`, replicated phase timer + HUD, handheld camera, VOIPTalker proximity voice, `TTA.Run.ScoreRound`.
- **Failed / fixed by hand:** not compiled in the sandbox; pending Kevin's build.
