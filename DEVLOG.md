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

## Task 30 — Title map plays black with build HUD
- Prompt: Title shows black with the build HUD; add a title GameMode/controller and rebuild the Title map lit with a camera.
- Agent: found Title had no GameMode override (fell back to TTAGameMode + TTABuildHUD) and no lights, and no runtime code created the menu; added ATTATitleGameMode/ATTATitlePlayerController, rebuilt create_title_map.py, host now travels to Backyard. Python tests only; UE not compiled in sandbox.

### Task 31 — Missing audio assets spam errors
- **Prompt:** missing `/Game/Audio/...` sounds: one warning per asset per session instead of an error on every play; `Scripts/create_placeholder_audio.py` (run by `setup_content.ps1`) makes silent placeholders unless a real file exists in `Content/Audio/`.
- **Agent did:** doctest repro for a warn-once log; root cause in `UTTAAudioDirector::LoadCue` (unconditional Error, no memory); `tta::MissingAssetLog`; quiet package check before load; placeholder importer + tests; setup step.
- **Failed / fixed by hand:** UE code and the importer not run here (no engine).

## Overnight — editor compile

- **Prompt:** the editor would not compile: `TTAMenuWidget.h` used `UEditableTextBox` inline while it was only forward-declared, and `TTATitleGameMode.cpp` failed around `GetGameInstance()`.
- **Agent did:** included `Components/EditableTextBox.h` (the join-IP click already used `GetText().ToString()`), included `Engine/GameInstance.h`, and rebuilt `TakeTwoAfterlifeEditor` Win64 Development. UMG, Slate, and SlateCore were already in `TakeTwoAfterlife.Build.cs`.
- **Failed / fixed by hand:** the rebuild succeeded on this machine (exit 0, no compiler warnings in that log). Nothing left for a person on this item.

## Overnight — content setup

- **Prompt:** delete `Content/Maps/Title.umap`, run `Scripts/setup_content.ps1`, and keep going until every step passes. Missing imported props should be placeholders.
- **Agent did:** the old backyard log failed because Python cannot see `TTAFinishGate.Arch` (it is not BlueprintVisible). The script now uses `get_editor_property`. Setup then built toon assets, an 80-prop Backyard of engine cubes (KayKit and Kenney are not downloaded), the toon swap, Title, and 76 silent sounds.
- **Failed / fixed by hand:** the pack download is still a manual step (`Scripts/fetch_assets.ps1`). Until those zips are in `Downloads/`, the backyard is coloured cubes, not the kit meshes. The sound import also logged a Bink decoder ensure; the SoundWave assets still came out at about 0.25 s.

## Overnight — Title map check

- **Prompt:** Title's game mode must be `TTATitleGameMode`, the scene must be lit, the menu must show, and the build HUD must not.
- **Agent did:** loaded the saved Title map in the editor. Game mode is `TTATitleGameMode`, HUD is the plain `HUD`, the controller is `TTATitlePlayerController` (it builds the menu widget on begin play), and the level has a sun, sky light, atmosphere, fog, and the tagged title camera.
- **Failed / fixed by hand:** nothing. This was not a play-in-editor look with eyes; it is the saved map plus the controller that creates the widget.

## Overnight — placeholder audio

- **Prompt:** run the task 31 placeholder audio script and confirm the assets exist.
- **Agent did:** setup created 76 silent SoundWaves (60 VO, 6 music, 10 SFX). A second run kept all 76. Loaded `SFX_Applause` and `MUS_Build` in the editor: duration about 0.25 s, music set to loop.
- **Failed / fixed by hand:** the first import logged "Decoder for AudioFormat BINKA not found" and the editor exited 1. The waves still imported. Real ElevenLabs / music files are still yours to drop in over these names.

## Overnight — tests

- **Prompt:** `make test`, then the TTA automation suite, and fix what fails.
- **Agent did:** `make` is not installed. CMake came from pip and the Makefile now passes `--config Debug` so Visual Studio's multi-config ctest actually runs. Two Core tests did not compile under MSVC (`1.0 / 0.0`, missing `M_PI`); both compile and the suite passes (43 Python tests too). Automation needed a game instance before `SetGameMode`, and the cinema clock now banks leftover milliseconds so a 30 fps step does not drift the 10 second rating. That cinema test passed on the last run. `TTA.Art.AllToon` passed.
- **Failed / fixed by hand:** after three automation runs the suite still fails and the last one crashes inside the menu widget test, so the rest never finish. Still red: build-place item meshes in DefaultGame.ini, the runner mesh for ragdoll, match premiere not advancing, respawn timing, and `TTA.UI.MenuFlow` crashing with no world. I stopped at the third attempt. Log: `Saved/Automation/TTA3.log`.

## Overnight — Shipping zip

- **Prompt:** package Win64 Shipping and zip it as TeamName_GameName, under 1 GB.
- **Agent did:** the first cook never started. Shipping links `TTACore` and the game module into one exe, and both were compiling the same Core sources. `TTACore` now compiles those sources only for the editor (separate DLL). The third `RunUAT BuildCookRun` succeeded. Zip is `Dist/ANonABento_TakeTwoAfterlife.zip`, 323.6 MiB.
- **Failed / fixed by hand:** the jam form may want a different team name than `ANonABento`. Rename the zip if so. Unzip and launch to Title yourself; I did not play the packaged exe.

## Overnight — compiler warnings

- **Prompt:** clean compiler warnings in our code.
- **Agent did:** checked the editor rebuilds and the Shipping build log.
- **Failed / fixed by hand:** there were no `warning C` lines to fix.

## Plan branch — submission docs, settings, heaven travel, menu

- **Prompt:** fold `plan/take-two-afterlife-mwff6y6w` into main (submission text, phase timings, fullscreen and resolution, heaven travel, menu restyle).
- **Agent did:** merged that branch. Kept the overnight devlog entries above. The other plan branch's task 28–31 notes described work that was already on main, so those notes were not copied over.
- **Failed / fixed by hand:** Heaven.umap still has to be built before premiere travel works. Font assets `LilitaOne` and `Nunito` are still not imported.

## MVP task 2 — No avatar or controls after Host
- **Prompt:** after Title → Host → Backyard?listen nothing is possessed; spawn and possess a runner in Lobby.
- **Agent did:** `ATTAGameMode` now uses `ATTARunnerPawn` as the default pawn (PostLogin/RestartPlayer spawn it at a PlayerStart, human form); Lobby without travel restarts runners; `ATTAPlayerController::BeginPlay` sets GameOnly input and hides the cursor. `Scripts/tests/test_runner_spawn.py` checks RunnerMesh, cooked maps, MoveForward/Jump mappings and this check list.
- **Failed / fixed by hand:** UE side untested in the sandbox; run the check below.

## Runner spawn check (user PC)
1. Package Win64 Development (`Scripts/package.bat` or Platforms → Package Project) with no errors.
2. Launch the packaged exe; the Title menu shows with a cursor.
3. Click Host; the Backyard loads and the menu and cursor are gone.
4. The white angel (SK_Angel) stands at the PlayerStart, not a grey sphere.
5. WASD moves, the mouse turns the camera, Space jumps.

## MVP task 4 — Bot pawns + Play Solo
- **Prompt:** bots that run the Backyard route to the gate, build and rate through BotBrain, auto-edit a cut; a Play Solo button that hosts Backyard with 3 bots.
- **Agent did:** `ATTABotController` (AI controller with a PlayerState, ids 1000+) possesses `ATTARunnerPawn`; steering is Core `tta::BotRoute` (marker to marker, jump at gaps and step-ups, respawn through `PawnLife` via `Hit()` when stuck 3 s); `BotBrain` builds in Build, `vote()` rates in the cinema, `rollFail()` makes some gaps fail on purpose. `ATTAGameMode` reads `?bots=N`, adds bots at StartPlay, keeps them through travel, runs them in Run and auto-edits their cut when Run ends. Title has PLAY SOLO → `UTTASessionSubsystem::HostSolo(3)`. Backyard route markers get the `TTARoute` tag. Automation test `TTA.Bots.SoloRound`; `Scripts/tests/test_bots.py`.
- **Failed / fixed by hand:** UE side untested in the sandbox. Re-run `py Scripts/build_backyard_map.py` so the markers get their tag, then run the check below.

## Solo round check (user PC)
1. Build the editor target; in Session Frontend run `TTA.Bots.SoloRound` and see it pass.
2. In the editor run `py Scripts/build_backyard_map.py`; select a bone stepping stone and see the `TTARoute` tag.
3. Launch (PIE or packaged) to Title and click PLAY SOLO; the Backyard loads with you and 3 other angels.
4. Press START; in Build, bot traps and cameras appear on the course.
5. In Run, the 3 bots run the stepping stones toward the gate and jump the gaps; a bot that falls or gets stuck respawns at the start.
6. Run ends (all at the gate or the timer); Edit opens and the round goes on to the Heaven podium.

## MVP task 5 — automation crashes

- **Fixed:** `TTA.UI.MenuFlow` crash. `UTTAMenuWidget::NativeOnInitialized` (run by `Menu->Initialize()`) built the Settings screen and called `RefreshVideo()`, which `check`ed `UGameUserSettings` and asked the RHI for fullscreen resolutions. In a bare automation world that assert takes down the whole run. Video options now fill in when Settings opens; `RefreshVideo`/`ApplyWindowMode`/`ApplyResolution` log and skip instead of `check`. The test's `ULocalPlayer` is now owned by the game instance, not `GEngine`. Locked by `Scripts/tests/test_menu_init.py`.
- **Rechecked, no change:** premiere advance. Core `MatchStateMachine` allows Premiere -> Build (or Podium on the last round); `ATTAGameMode::EndPremiere` asks exactly that and `TTA.Match.ThreeRounds` drives it. In-game, nothing calls `EndPremiere` yet (cinema not wired: separate task).
- **Rechecked, no change:** respawn timing. `ATTARunnerPawn::AdvanceLife` slices dt at 0.05 s so `PawnLife` walks Frozen (0.12 s) -> Ragdoll (1.5 s) -> Alive even when a test passes the whole delay at once; ticks use world dt so the frozen pawn still counts down.
- **Untested here:** UE compile and Automation (no Unreal in the sandbox). On Windows, in the editor console: `RunTests TTA.` (or `UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -ExecCmds="Automation RunTests TTA.;Quit" -unattended -nullrhi -log`).


## MVP task 9 — premiere ends
- **Cause:** nothing in game started the Heaven `ATTACinema` or listened for its end, so `ATTAGameMode::EndPremiere` was never called and the match sat in Premiere forever.
- **Fix:** on arrival in Heaven (`PostSeamlessTravel`, or right away if the travel fails) the server runs `BeginPremiere`: finds the level's cinema, gives it the bots' auto-edited cuts, binds `OnStep`. Bots vote on each Rating step. Podium/Done calls `EndPremiere` with the cinema's rating points. A timeout (premiere length + `PremiereTimeoutSlack` 3 s; just 3 s with no cinema or no cuts) ends it anyway. Build-item meshes are now KayKit Prototype Bits. Locked by `Scripts/tests/test_premiere_end.py`.
- **Untested here:** UE compile and play (no Unreal in the sandbox).

## Solo round to the Heaven podium (user PC)
1. Build the editor target. Launch to Title, set rounds to 1 in host settings (or play all rounds), click PLAY SOLO.
2. START; Build, Run and Edit play out with the 3 bots.
3. Heaven loads; the cinema plays each bot cut: intro, playback, rating (bots vote), reveal.
4. After the last cut the server calls `EndPremiere`; on the final round the phase goes to Podium and the Heaven podium shows the totals. Earlier rounds go back to the Backyard Build.
5. In a test copy of Heaven with the cinema actor deleted, repeat: the premiere ends 3 s after Heaven loads and the round still reaches the podium.
6. Build items in Build show KayKit pieces, no grey engine cubes/cones.

## MVP task 5 (plan 2) — editor won't compile on UE 5.5
- **Cause:** the code leaned on transitive engine includes (no PCH, no unity) that UE 5.5 doesn't give, so `FHitResult`/`FCollisionQueryParams`/`UStaticMeshComponent` were undeclared. `ATTAGameMode::EnterRun` had `if (...) UE_LOG(...); else ...`; `UE_LOG` expands to a braced block, so the `;` orphaned the `else` (C2181). `TTA.UI.MenuFlow` had its `;` inside a trailing `//` comment, so the next `const` line hit C2143.
- **Fix:** IWYU includes in the four reported files plus every Source file the bots/menu/cosmetics/look/premiere tasks touched (`Engine/HitResult.h`, `CollisionQueryParams.h`, `Components/StaticMeshComponent.h`, `GameFramework/PlayerController.h`, `Engine/World.h`); braces on that if/else; comment moved off the statement. `TakeTwoAfterlife.Build.cs` public deps now list AIModule, NavigationSystem, GameplayTasks, UMG, Slate, SlateCore, EnhancedInput. Locked by `Scripts/tests/test_ue55_compile.py`.
- **Untested here:** real UE compile (no Unreal in the sandbox). Confirm on your PC from the repo folder in PowerShell:

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" TakeTwoAfterlifeEditor Win64 Development -Project="$PWD\TakeTwoAfterlife.uproject" -WaitMutex -NoHotReload
```
It should end with `Result: Succeeded`. If not, paste the first `error C` lines.

## C2487 in TTAUIStyle.h (UE 5.5)
- Cause: `static constexpr int32 kH1 = 120, kH2 = 64, ...;` declared several static members in one statement inside the exported `UTTAUIStyle`; MSVC reports C2487 on every name after the first (kH2, kTimer, kBody, kSmall, kOutline, kCardPadding). No member had its own API macro and there were no .cpp definitions to delete.
- Fix: one `static constexpr` per line, values unchanged. `Scripts/tests/test_exported_members.py` checks every `TAKETWOAFTERLIFE_API` class/struct in Source/ for per-member API macros and comma-listed static members.
- Confirm on your PC (PowerShell, repo root):

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" TakeTwoAfterlifeEditor Win64 Development -Project="$PWD\TakeTwoAfterlife.uproject" -WaitMutex -NoHotReload
```

## apply_world_look.py on UE 5.5
- Cause: `Scripts/apply_world_look.py` `main()` set MPC values through `unreal.KismetMaterialLibrary` on `unreal.EditorLevelLibrary.get_editor_world()`; neither is exposed in UE 5.5 Python, and a runtime world value would not be kept in packaged builds anyway.
- Fix: `write_mpc_defaults()` writes the Course preset from `[/Script/TakeTwoAfterlife.TTAWorldLookSettings]` into the `MPC_TTA_World` asset's `scalar_parameters` (and `vector_parameters` if a preset ever has them) default values and saves it with `EditorAssetLibrary.save_asset`. Heaven still switches at runtime via `UTTAWorldLookLibrary::ApplyWorldLook`. Editor world comes from `UnrealEditorSubsystem`. Outline volume still goes into Title, Backyard and Heaven. `Scripts/tests/test_world_look_ue55.py` bans `unreal.Kismet*` / `EditorLevelLibrary` in Scripts/.
- Confirm on your PC:
  1. Open the project in UE 5.5, Output Log -> Python: `py Scripts/apply_world_look.py`. It should log one "look volume" line per map and `MPC_TTA_World: Course defaults saved`, with no errors.
  2. Close and restart the editor.
  3. Open `/Game/Art/Toon/MPC_TTA_World`: Saturation, Contrast, ShadowFloor, Bands, RimStrength, Warmth must still show the Course values from `Config/DefaultGame.ini`. Open Title, Backyard and Heaven: each has one `TTAWorldLook_*` PostProcessVolume.

## Plan 3 task 2: Bots don't run the round
- **Found:** `ATTABotController` already had `bWantsPlayerState = true`, straight-line `BotRoute` steering (no NavMesh anywhere) and EndRun -> Edit with `FillCut` per bot. The "runner has no PlayerState" log came from `UTTAMicRecorder`: AI pawns are locally controlled on the server, so each bot's lobby runner opened the mic at Run; EnterRun then unpossessed and destroyed that pawn, and `StopAndSave` read the (now null) PlayerState at EndPlay.
- **Fix:** mic records only player-controlled pawns and takes the PlayerState id when capture starts. Bot steering logs under `LogTTABots` (start line + once a second at Verbose). `Scripts/tests/test_bots_round.py`.
- **To run (UE 5.5 editor console or Session Frontend):** `Automation RunTests TTA.Bots`. If "bot moved toward the gate" still fails, run `log LogTTABots Verbose` first and send the LogTTABots lines.

## Menu test, resolution and materials (plan 3 task 3)
- MenuFlow test now builds its world from `UGameInstance::InitializeStandalone()` (LocalPlayer outer = an initialized game instance, no ensure).
- Resolution index is 0-based and clamped via `tta::clampResolutionIndex` (no more "choice 17 out of range (17 choices)").
- Runner body MIDs only for slots the loaded SK_Angel has (`RunnerMesh=/Game/Characters/Angel/SK_Angel.SK_Angel`); hat MID guarded.
- KayKit `prototype_texture` -> Masked: in the editor, `py Scripts/fix_prototype_material.py`.
- **To run (UE 5.5):** `Automation RunTests TTA.` — TTA.UI.MenuFlow expected to pass. Untested here (no Unreal in the sandbox).

## Plan 4 task 1: Heaven lobby + gate start
- **Did:** Heaven is the lobby (`TTASessionSubsystem::LobbyMap`). Host / Join / Play Solo land there as `ATTARunnerPawn`. The host's START (Enter or gamepad Menu) opens the gate; Solo opens it at StartPlay and its 3 bots count as ready. `ATTAFinishGate` in Lobby marks the player ready. `tta::HeavenGate` (Core, doctest) decides when to travel: all ready, or 20 s after the first player through. GameState replicates `bGateOpen`, `ReadyCount`, `ReadyMax`; the HUD shows "n/max READY". `dress_heaven.py` places a `TTAFinishGate` on the red carpet.
- **Manual check (UE 5.5, untested here):** 1. `py Scripts/dress_heaven.py` once. 2. Title -> Host: you spawn in Heaven and can walk. 3. Press Enter (START): a "0/1 READY" card shows. 4. Walk or jump through the gate on the red carpet: "1/1 READY", then Backyard loads in Build. Solo: the card shows "3/4 READY" at once; going through the gate loads Backyard. With two players, one through the gate -> Backyard after 20 s.


## Plan 5 task 3: Score board, podium statues, back to lobby
- **Did:** after each cut's Funny/Evil vote the premiere's 4 s Reveal step now shows a Kahoot-style board: bars slide from the old totals to the new ones and rows swap to the new rank (`tta::scoreBoard` / `scoreBarAt`, drawn by `ATTABuildHUD::DrawScoreBoard`). After round 3 `tta::PodiumShow` runs: travel to Heaven if not there, the top 3 stand on the podium as frozen angels (hat, face, palette colour, animation paused), `PodiumHoldSeconds` (8 s) later everyone is back in the Heaven lobby without a map reload, so the statues stay until the next match's Build.
- **Manual check, solo round (UE 5.5, untested here):** 1. `py Scripts/dress_heaven.py` once (podium TargetPoints tagged 1st/2nd/3rd). 2. Set `Rounds` to 1 in the host settings (or play 3 rounds). 3. Title -> Play Solo, go through the gate, build, run, edit. 4. In the Heaven premiere, after each cut's rating screen a score board shows for ~4 s: bars grow and rows reorder, then the next cut starts. 5. After the last cut: HUD shows PODIUM, three angel statues in their hats/colours stand frozen on the podium spots. 6. ~8 s later the phase is LOBBY, you have a runner in Heaven, the statues are still there. 7. Press Enter, go through the gate: Backyard loads and the statues are gone.

## Plan task 4: Proximity voice chat
- **Did:** UE 5.5 VOIP was already on in config (`[OnlineSubsystem] bHasVoiceEnabled`, `[Voice] bEnabled`, `DefaultPlatformService=NULL`, open mic `bRequiresPushToTalk=false`). Each remote runner gets a `UVOIPTalker` registered to its PlayerState; its attenuation is now full volume within 5 m and silent past 25 m (`VoiceFullVolumeCm` / `VoiceSilentCm` in DefaultGame.ini). [M] toggles your outgoing voice (`ToggleSpeaking`); the TTAMicRecorder replay track keeps recording while muted. `Scripts/tests/test_proximity_voice.py`.
- **Two-window test (UE 5.5, untested here):** 1. Editor: Play -> Number of Players 2, Net Mode "Play As Listen Server", "Run Under One Process" off (two processes so each opens the mic). Or two PCs on one LAN: Host on one, Join with the code on the other. 2. Use headphones on at least one side (or two PCs) to avoid echo. 3. Stand next to each other in Heaven and talk: clear, full volume. 4. Walk apart: volume holds to ~5 m, then fades, and is gone past ~25 m. Sound pans with direction. 5. Press M on the talker: the other side hears nothing; press M again: voice is back. 6. Muted during a Run: the premiere still plays your mic track (Saved/Mic/Round<R>_Player<P>.wav exists). If nothing is heard at all, check `LogVoice` in the Output Log and send those lines.

## Plan 6 task 5: Title menu simplified
Title is Play Solo / Host Game / Join (code or IP) / Settings / Quit. Logo and button art slots added to TTAUIStyle (DefaultGame.ini) so generated art drops in with no code change. Check: build, open the Title map, confirm 5 buttons each work; set MenuLogo to a texture and confirm it replaces the text title.

## Build watch: automation green on UE 5.5 (Nov 15)
- Editor build of main (fbaaf9f) succeeded with no changes.
- `TTA.Bots.SoloRound`: the test called `World->Tick` 840 times inside one engine frame; tick functions queue once per `GFrameCounter`, so only the first tick ran (bots never moved, Run never ended). The test now bumps `GFrameCounter` per simulated frame. Game code untouched.
- `TTA.UI.MenuFlow`: `ULocalPlayer` is `Within=Engine`; outering it to the game instance tripped an ensure. The test now outers it to `GEngine`; `test_menu_init.py` asserted the opposite and was corrected.
- **Verified on this PC:** Build.bat exits 0 (all 22 actions, DLL linked); `Automation RunTests TTA.` 16/16 pass; `Scripts/test.ps1` green (doctest + 180 Python tests).

## Merge main into plan branch (Nov 15)
- Merged `origin/main` (build-watch test fixes) into the plan branch. Only DEVLOG.md and plan.md conflicted; both sides' entries kept. The three test files merged cleanly.
- First real UE 5.5 compile of the plan-branch features needed two fixes: `TTAGameMode.cpp` includes `UObject/Package.h` (`IsOnMap` calls `GetOutermost()->GetName()`), and `InputCore` is now a module dependency (`EKeys::Enter` / `Gamepad_Special_Right` for the host's START key failed to link).
- **Verified on this PC:** Build.bat exits 0 (DLL linked); `Automation RunTests TTA.` 16/16 pass headless (`-nullrhi`); `Scripts/test.ps1` green (doctest + 196 Python tests). The manual checks listed above (lobby gate, 3 rounds, podium, voice, menu) are still unplayed.

## Backyard fit: kit units and axes (plan task 7)
The patio floor tile (KayKit `Primitive_Floor`, imported as 4 x 1 x 4 cm: metres read as cm, Y-up) stopped the Backyard build at the fit guard. Fits now apply a x100 unit fix for sub-10 cm meshes and roll Y-up slabs 90 deg about X before the per-axis fit; a prop that still can't fit is logged and skipped instead of aborting the build (Backyard and Heaven). Needs an editor run to confirm the slab lands flat.

## Lobby spawn + camera check (user PC)
Code only; Heaven.umap untouched. Needs an editor compile first. To keep `dress_heaven.py` off the hand-dressed map, add the actor tag `TTA_HandDressed` to Heaven's World Settings (or any actor) and save.
1. Open Heaven, press Play Solo from the title menu.
2. The angel appears about 3 m above the Heaven floor and drops onto it; it does not start inside a cloud or under the floor.
3. The camera is behind the angel and turns with the mouse.
4. Back the angel into a cloud or the gate: the camera slides in toward the angel instead of going inside the mesh.
5. Hold RaiseCamera: the view switches to the handheld; release: back behind the angel.

## UI art + shared style system (UI refactor task 1)
Generated UI art (18 alpha PNGs: buttons, panel, code pill, timer banner, slider, text field, mic/camera/ready icons, title backdrop) is in `Art/UI/`, with `Scripts/import_ui_assets.py` to bring it into `/Game/UI/Generated/`. `UTTAUIStyle` now has named tokens (`Color`, `Font`, `Space`) and builders (`MakeButton`, `MakePanel`, `MakeLabel`, `MakeIconCount`, `MakeBanner`, `MakeSlider`, `MakeTextField`), listed in `docs/design/ui-style.md`. The edit screen is built from them. Check on the user PC:
1. `UnrealEditor-Cmd.exe TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/import_ui_assets.py -unattended -nosplash`; `/Game/UI/Generated` has 18 `T_UI_*` textures, Mip Gen = NoMipmaps, group UI.
2. Editor build passes.
3. Edit phase: gold AUTO-EDIT, white buttons, timer banner, framed caption field, white panel behind the tool row; no stretched corners (else tune the slice in TTAUIStyle.cpp and ui-style.md together).
## Plan task 1 — Toon outline misalignment
- **Prompt:** outlines sit offset from mesh edges; fix the outline post-process and get world normals into the GBuffer.
- **Agent did:** `OUTLINE_HLSL` now converts with `ViewportUVToBufferUV(UV)` and samples depth/normal at `buv`, stepping by `View.BufferSizeAndInvSize.zw` (was viewport UV + viewport pixel size, wrong whenever buffer != viewport: editor viewport, screen percentage, split screen). Scanline/vignette keep viewport UV. `M_TTA_Toon` is now Default Lit with the toon result in Emissive and black BaseColor, so it writes world normals. Static tests in `Scripts/tests/test_toon_outline.py`.
- **Failed / fixed by hand:** not yet checked in the editor: rebuild with `Scripts/setup_content.ps1` and look at TTA_ArtTest.

## Heaven props on the cloud floor
Props were placed at world Z 0 instead of the cloud top (the cinema actor's origin), and dress_heaven.py crashed on a missing `mesh_bounds`. Placement now goes through the cinema transform; the podium slab sits 0.5 cm up to stop z-fighting. Editor rerun still needed.

## Punch rules in Core
Added `tta::Punch` (wind-up/active/recovery + 1.5 s cooldown) and a PawnLife stun: 1 s ragdoll, get up in place, 2 s punch immunity; a trap hit during a stun still respawns. Doctest only; UE wiring is the next task.

## Punch task 2 — Punch on the pawn and online
- **Prompt:** LMB/gamepad punch with Server RPC, server sphere overlap during Active, Multicast ragdoll + impulse, get up in place.
- **Agent did:** `Punch` input, `ServerPunch`, server `AdvancePunch`/`PunchHitCheck`, `MulticastPunched`, `StopStun` (capsule sweep to pelvis), shared `RestoreBody`; static test `test_punch_net.py`.
- **Failed / fixed by hand:** punch immunity never ticked (life loop stopped at Alive); loop now runs while immune. Editor compile + PIE host/client test pending.

## Punch task 3 — readable wind-up
- **Prompt:** wind-up pose or scale-up placeholder, whoosh + hit cues via AudioCues, short camera shake on the victim.
- **Agent did:** `AudioEvent::PunchWindUp`/`PunchHit` with `SFX_PunchWhoosh`/`SFX_PunchHit` (doctest). Server `ServerPunch` now fires `MulticastWindUp` (Unreliable) when the punch starts: every client swells the runner mesh to `WindUpScale` 1.15 over the 0.45 s wind-up and plays the whoosh. `MulticastPunched` plays the hit and starts `ImpactShake` on the victim's own camera.
- **Failed / fixed by hand:** no SFX files yet; run `create_placeholder_audio.py` (silent placeholders) or drop real ones in `/Game/Audio/SFX/`. Editor compile + PIE feel check pending.

## Audio drop-in (music + SFX)
- **Prompt:** real CC0 music and SFX for the 16 cue names, 44.1 kHz WAV, about -16 LUFS music / -12 LUFS SFX, CREDITS rows, captions only for VO.
- **Agent did:** `Scripts/build_audio.py` downloads six CC0 OpenGameArt loops (In the Lobby, Happy Adventure, Lasso Lady, Menu Music, Heavenly Loop, Funky Victory Loop), cuts them to 32 s mono so each stays under 3 MB, renames the `make_sfx.py` synth pack to the `SFX_*` cue ids, and loudness-normalises everything with ffmpeg into `Content/Audio/{Music,SFX}/`. CREDITS rows filled in; WeLoveIndies/Freesound rows dropped. Test: `Scripts/tests/test_audio_dropin.py`.
- **Failed / fixed by hand:** kenney.nl returns 403 to the sandbox, so SFX come from the in-house synth pack. Music longer than 32 s no longer loops seamlessly. On the PC: run `python Scripts/build_audio.py`, drag the WAVs into `/Game/Audio/Music` and `/Game/Audio/SFX` (replace the silent placeholders), tick Looping on the six MUS_ waves, then PIE.

- 2026-11-15: Narrator VO generated with ElevenLabs (voice George, eleven_multilingual_v2): 60 `VO_*` WAVs, 44.1 kHz mono, -16 LUFS. Unzip into `Content/Audio/VO/` and import over the silent placeholders. `Scripts/tests/test_vo_audio.py` checks names match `AudioCues.cpp`.
- 2026-11-15: Imported all 76 real sounds over the placeholders with `Scripts/import_audio.py` (headless; sets Looping on `MUS_*`). `ImpactSound` left unset: there is no runner pawn Blueprint and `SFX_ImpactBonk` already plays through the Wipeout cue. `SFX_PunchWhoosh`/`SFX_PunchHit` are still silent placeholders. PIE listen-through pending.
## Single fixed camera
Cut placeable cameras to Fixed only, cap 8 per player. Other camera types stay in code/assets but can't be placed.
## Trap swap task 2 — trap size and pivot check
- **Prompt:** record each `SM_Trap_*` bounding box plus a 1.0 x 0.6 x 0.5 m lawnmower box; verify exports are within +-10% with the pivot at the origin.
- **Agent did:** `models/trap_bounds.json` + `models/trap_fit.py`; `verify_traps_cams.py` now fails a trap that is off-size or off-pivot. Unittest `test_trap_bounds.py` (placeholders pass, 2x scale fails).
- **Failed / fixed by hand:** boxes read from `traps_cams.glb`; the Blender run on the FBX files is still pending.

## Trap swap task 3 — lawnmower core
- **Prompt:** add a one-shot Lawnmower trap (Idle -> Driving -> Spent, 3 x 1.5 m trigger zone, 4 m at 4 m/s, hits through the existing path) and hide Cannon from placement and bots with `BadItem`.
- **Agent did:** `tta::Lawnmower` + `TrapType::Lawnmower`, `trapPlaceable()` / `PlaceResult::BadItem`, bots and UE build menu skip the cannon. Doctests in `LawnmowerTests.cpp`.
- **Failed / fixed by hand:** UE side not compiled here; the mower does not move in game until the UE hookup task.

## Trap swap task 4 — import and fit downloaded traps
- **Prompt:** import the CC0 trap downloads, rotate/pivot/scale them to the recorded boxes, recolour from palette.md, export as `SM_Trap_*` plus a new `SM_Trap_Lawnmower`; primitives when a file is missing.
- **Agent did:** `trap_fit.THIRDPARTY` + `fit_matrix` + `palette()`, import step in `build_traps_cams.py`, procedural lawnmower, `fetch_traps.py`, CREDITS rows and a third-party credits rule in `check_credits.py`; unittests in `test_trap_bounds.py`.
- **Failed / fixed by hand:** downloads blocked (403) in the sandbox; fetch and commit the .glb files locally and check orientation in the preview.

## Lawnmower in UE (trap swap task 6)
Trap actor steps the Core lawnmower each tick, hits go through TryHit, blade spins while driving; build item mesh is Pallet_Small instead of the cannon barrel.
## UI redo task 1 — drawn chrome
Buttons, panel, pill, banner, text field and slider art are now drawn by `Scripts/build_ui_chrome.py` at fixed sizes (all buttons 512x160) with one 6 px Ink outline and 4 px shadow; 9-slice margins are written to `Art/UI/T_UI_Slices.json` and read by `UTTAUIStyle` instead of typed-in fractions. Editor compile + reimport pending.

## UI redo task 2 — same-size icons
`Scripts/normalize_ui_icons.py` trims faint alpha noise off every `T_UI_Icon*.png` and centres the glyph on a 256x256 canvas with 24 px padding, so icon buttons are all one size. Editor reimport pending.

## UI redo task 3 — one type scale
Type sizes now have one source: `Docs/design/ui-style.md` Fonts table = `UTTAUIStyle::kTitle/kH1/kH2/kBody/kSmall` (120/96/64/32/24), checked by `test_ui_style_guide`. Old `kTimer` and the menu's private size constants are gone; on-screen px are unchanged. Editor compile still needed.

## UI redo task 4 — title, join, how to play
Title now sits top-left inside the safe area: logo, then a five-button stack all `kButtonW` wide, drawn with the style-guide button art (`UTTAUIStyle::MakeButton`), Host as the one gold Primary. Join and How to Play are each one centred panel with a heading and Back inside. `test_menu_structure` checks order, equal widths, Primary and safe-area margins. Editor compile + in-game look pending.

## UI redo task 5: lobby player cards
Lobby text lines replaced by code pill, 4-slot player card row (avatar, name, ready tick) and Customize/Start bottom row; `SetPlayerCards` + structure test. Untested in editor.


## Get up where you landed
StopStun ran on every machine with its own pelvis read and a sweep from the pre-ragdoll capsule, so the sweep got blocked and replication pulled runners back to where they stood. Now the server traces the pelvis to the floor and sends the spot through MulticastGetUp; everyone restores the mesh and teleports the capsule there. Trap path untouched. Static test: Scripts/tests/test_get_up_landing.py. Needs an editor compile and a host+client check.
