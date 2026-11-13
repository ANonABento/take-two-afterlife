# Take Two Afterlife — Dev Log

Built for the Epic AI MegaJam 2026 (theme: Director's Cut). Each entry: the prompt, what the agent did, what failed, what was fixed by hand.

## Task 1 — Asset sourcing research
- **Prompt:** find human-made, jam-legal asset sources for UE5 with at least 15 cited sources and a shortlist.
- **Agent did:** browser research; wrote `assets-ue5.md` with sources and a shortlist (Kenney, Quaternius, KayKit, Poly Haven, Freesound, Fab free).
- **Failed / fixed by hand:** nothing.

## Task 2 — Party-game UI reference board
- **Prompt:** Jackbox / Fall Guys style UI reference: palette hex list, fonts, per-screen spec.
- **Agent did:** browser research; wrote `ui-party-games.md`.
- **Failed / fixed by hand:** its palette later lost to the repo's `toon_spec.py` palette (task 24).

## Task 3 — EOS join codes + voice + mic research
- **Prompt:** EOS lobby join codes, P2P voice and mic capture in UE5, with setup checklist and fallback.
- **Agent did:** browser research; wrote `eos-ue5.md` (setup checklist, direct-IP fallback).
- **Failed / fixed by hand:** nothing.

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

## Task 11b — Toon shader + outline
- **Prompt:** one master `M_TTA_Toon` material, post-process outline, `MPC_TTA_World` presets, swap script, Automation TTA.Art.AllToon.
- **Agent did:** editor Python that builds the palette texture, MPC, toon + outline materials and an art-test map; `swap_to_toon.py`; `toon_spec` Python tests.
- **Failed / fixed by hand:** no UE in the sandbox: material HLSL and editor Python untested until Kevin's run.

## Task 12 — Angel character
- **Prompt:** 1.2 m big-head angel with painted face, nub hands/feet, wings and halo, palette UVs.
- **Agent did:** scripted Blender build (`models/build_angel.py`) exporting `SK_Angel`, wings, halo, default face and palette textures, plus turnaround renders.
- **Failed / fixed by hand:** proportions checked from renders; import into UE by hand.

## Task 13 — Hats + faces kit
- **Prompt:** hats and swappable face textures so human forms are distinguishable.
- **Agent did:** scripted Blender build of 6 hats (chef, cowboy, firefighter, hard hat, pilot, police) and 6 faces, plus lineup renders.
- **Failed / fixed by hand:** first hat script replaced by `build_hats2.py`.

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

## Task 20 — Mic capture
- **Prompt:** record each runner's mic to file during Run keyed to replay time, playback API, silent fallback; Automation TTA.Audio.MicSync (offset < 50 ms on fixture).
- **Agent did:** Core `MicTrack` (+ doctest), `UTTAMicRecorder`, `UTTAMicPlayer`, replicated phase start for the client replay clock, `TTA.Audio.MicSync`.
- **Failed / fixed by hand:** `make test` needed cmake installed in the sandbox; UE code not compiled here, pending Kevin's build.

## Task 21 — Editor screen
- **Prompt:** trim, reorder, multi-angle, slow-mo, zoom, stickers, captions; 30 s cap; auto-edit fallback.
- **Agent did:** Core `CutPlayback` (maps cut time to replay time and camera) + doctest, UE editor widget and preview playback.
- **Failed / fixed by hand:** UE code not compiled here, pending Kevin's build.

## Task 22 — Heaven cinema + ratings
- **Prompt:** walkable heaven cloud, MPC heaven preset, 4-seat couch, big screen, random order, narrator intro, playback with runner mic, 10 s rating, reveal, podium; Automation TTA.Cinema.Sequence.
- **Agent did:** Core `Premiere` sequence (+ doctest), `RatingBook::averages`, `ATTACinema` actor, `TTA.Cinema.Sequence`.
- **Failed / fixed by hand:** UE code not compiled here, pending Kevin's build.

## Task 23 — Full match flow
- **Prompt:** game mode drives Core through 3 rounds Build→Run→Edit→Premiere→Podium, join between rounds, leaver traps persist, host settings JSON; Automation TTA.Match.ThreeRounds.
- **Agent did:** Core MatchFlow (join rule, leaver trap owners, id mapping) + absent-owner scoring + `addRatings` (doctest), host settings file + replication, timer auto-advance, `EndPremiere`, `TTA.Match.ThreeRounds`.
- **Failed / fixed by hand:** UE code not compiled here, pending Kevin's build.

## Task 24 — Menus + UI skin
- **Prompt:** Title → Host/Join → Lobby → Match menus, settings, how-to-play, Fall Guys-style skin; Automation TTA.UI.MenuFlow.
- **Agent did:** Core `MenuFlow` screen stack + customization + settings clamp (doctest), C++-built `UTTAMenuWidget`, `TTA.UI.MenuFlow`.
- **Failed / fixed by hand:** fonts to import by hand; widget animations not done. UE code not compiled here.

## Task 25 — Narrator + music + SFX
- **Prompt:** ~60 ElevenLabs line slots with a phase trigger table, WeLoveIndies music per phase, SFX hookups, attributions in CREDITS.md; Automation TTA.Audio.TriggerTable.
- **Agent did:** Core `AudioCues` table (60 narrator lines with script, 6 music, 10 SFX; round-robin picks; asset paths) + doctest, `tta_vo_script` CSV export for ElevenLabs, `UTTAAudioDirector` world subsystem (phase music crossfade + phase lines, `PlayEvent`), hooks for wipeout/gate/smite, `TTA.Audio.TriggerTable`, CREDITS rows.
- **Failed / fixed by hand:** audio files can't be generated or downloaded from the sandbox (ElevenLabs account, WeLoveIndies licence); Kevin imports them. UE code not compiled here.

## Task 26 — Release package + devlog
- **Prompt:** final Win64 zip under 1 GB, DEVLOG.md complete and exported for the itch devlog, submission tools list.
- **Agent did:** added Backyard to the cooked maps, back-filled devlog entries for every plan task, `Scripts/export_devlog.py` (fails if a plan task has no entry, writes itch-ready HTML) with tests, `SUBMISSION.md` checklist and filled the CREDITS tools table.
- **Failed / fixed by hand:** packaging and the launch-to-Title check need Kevin's Windows machine.

## Task 28 — Content bootstrap fix
- Prompt: fix `create_toon_assets.py` TypeError in `-run=pythonscript`; default Downloads/ and /Game; placeholders for missing packs; add fetch_assets.ps1 + setup_content.ps1.
- Agent: reproduced with a fake UE struct (no-arg constructor), fixed `custom_node`, added the defaults, placeholder blocks and both PowerShell scripts, with unit tests.
- Failed / by hand: the TypeError only showed on Kevin's machine (UE structs reject constructor kwargs); the in-engine run is still to be checked by hand.
