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
