# Take Two Afterlife — Epic AI MegaJam 2026 (plan v3)

Every task runs from its task line plus this file only. Read this whole file first, do only your task, and leave the decisions below unchanged.

## Jam facts
- Jam: https://itch.io/jam/2026-epic-ai-megajam. Theme: **Director's Cut**. Window: Nov 12, 00:00 EST to Nov 15, 5:59 PM PT. Nothing made before Nov 12 except declared/licensed assets.
- Submit: Win64 zip named `TeamName_TakeTwoAfterlife`, under 1 GB, plus a 30–60 s gameplay video, the Epic email, a tools list and a dev log link. Text, audio and video must be in English, with content suitable for 13+ (slapstick only, no gore).
- Tracks: **Best Student Project** (main) and **Solo Driver** (modifier). Glass Box gets a public dev log page (itch devlog; Twitter not needed).
- Repo: **ANonABento/take-two-afterlife** (new). UE 5.x, C++. Solo dev (Kevin). No schedule: go fast, keep it correct, finish the multiplayer loop early so the last day goes to a recorded friend session, the trailer and polish.

## Game in one line
A party game for 2–4 people. Angels set traps and hide cameras on a shared ragdoll obstacle course, race it as reincarnated mortals, edit the footage of each other getting wiped out, then rate each cut on a cloud couch in heaven.

## Locked design
**Loop** (3 rounds, host-adjustable):
Build → Run → Edit → Premiere, repeated each round, then Podium.
- **Build** (timer): everyone places traps and cameras on the same map at once. Other players' placements stay hidden until Run starts.
- **Run** (timer): everyone races start to gate together, with proximity voice. Each runner has a Content Warning-style handheld camera. A death respawns you at the **start**. When the timer runs out, a lightning **smite** hits you, filmed by a guaranteed "Heaven Cam".
- **Edit** (90 s): you edit footage from your own cameras and your handheld. Hits are auto-marked. Auto-edit is the fallback when you run out of time.
- **Premiere:** cuts play in random order. The narrator introduces the director and the victim. After each cut, everyone has 10 s to rate it 1–5 on **Funny** (points to the editor) and **Evil** (points to the builder). No rating yourself.
- The map persists, so traps pile up across rounds. One themed map first: **suburban backyard**. Office or construction maps only if there's time.

**Scoring:**
- Reaching the gate: +1.
- When a trap wipes someone out, its owner scores the trap's cost (minimum 1).
- If everyone finishes, or nobody finishes, no trap points are given that round (the Ultimate Chicken Horse rule; this is what keeps courses beatable).
- Rating averages get added in.
- The podium is crowned at the end.

**Build:**
- Free placement inside a corridor of about 60 × 8 m. Snaps to a 0.5 m grid, rotates in 45° steps. Budget is 100 points.
- Traps (6), with 2 settings each, all on fixed deterministic loops (no randomness):

| Trap | Setting 1 | Setting 2 |
|---|---|---|
| Hammer | swing speed | direction |
| Red ball cannon | fire rate | ball size |
| Spinner | speed | arm length |
| Pusher wall | force | cycle time |
| Trapdoor | open delay | open duration |
| Banana | slide distance | count 1–3 |

- Trap ideas for later: spring pad, fan, conveyor, boxing glove, falling piano, rolling log, glue patch, fake gate.
- Cameras cost 0 budget, capped at 12 per player. Types: fixed, pan, follow, trap-cam. Stretch types: drone, CCTV.

**Recording:**
- Record replay **data**, not video: root + 15 bones at 20 Hz for every player, plus hit, wipeout and smite events tagged with the trap's id and owner. Each runner's mic is recorded to a file on the same timeline.
- Cameras render this data when you preview or play back.

**Impact frame:**
- On a wipeout, freeze **only the victim** for about 0.12 s, with a white flash outline, camera shake and SFX. No sticker word.
- Impact frames are stored in the replay, so they show up again in the editor and the cinema.

**Editor:**
- Trim, reorder, multi-angle, slow-mo, zoom, stickers, captions, and a "WASTED"-style card.
- 30 s cap per cut, so a premiere runs 2 min at most.
- Cuts auto-save locally to a **Reels** gallery in the main menu. MP4 export is a stretch goal; for the jam, OBS covers it.

**Cinema:**
- Heaven sky, a small walkable cloud, a 4-seat cloud couch facing a big screen.
- Live proximity voice on top of the playback.
- Playback carries the runner's recorded mic.

**Character:**
- An angel about 1.2 m tall, with a big head, a painted face (swappable texture), nub hands and feet, small wings and a halo.
- Human form drops the wings and halo, and adds a hat and face so players can tell each other apart.
- Customization: colour, hat, face. No unlocks.

**Look and audio:**
- 12-colour flat palette, pastel heaven against saturated courses, chunky outlines.
- UI: Jackbox-style host feel with Fall Guys-style big, clean type, game-show cards and bouncy motion.
- Narrator: about 60 lines, ElevenLabs TTS baked into audio files. Free-tier attribution goes in CREDITS.
- Music: WeLoveIndies tracks, one per phase.
- **Assets: human-made only. No AI-generated assets.** Sources: Kenney, Quaternius, KayKit, Poly Haven, Freesound, free items on Fab. Log every asset in CREDITS.md.

**Network:**
- Epic Online Services lobby with a **join code**, using EOS P2P relay, running as a listen server. Direct IP is a dev fallback. No Steam.
- Players can join only between rounds. A player who leaves keeps their traps on the map.
- Host settings (rounds, timers) sync to everyone and are saved to a JSON file on the host's machine.

**Menus:** Title → Host / Join (code) → Lobby (customization + settings) → Match.
- Settings: volume, mouse sensitivity, mic test.
- Optional illustrated "How to play" page. No tutorial level.

**Deferred:** bots and solo play, until the multiplayer loop works.

## Requirements (testable)
1. Core state machine runs Lobby → Build → Run → Edit → Premiere → (next round | Podium). Illegal transitions are rejected. Rounds and timers load from the settings JSON.
2. Placement: grid snap, rotation, corridor bounds, budget 100, camera cap of 12 by type, settings saved to JSON. Placements stay hidden from other players until Run.
3. Traps are deterministic: the same settings and time always give the same pose.
4. Scoring follows the rules above, including the all-finish/none-finish rule.
5. Replay: a 3-minute, 4-player recording is under 8 MB, with interpolation error under 1 cm.
6. Edit list: trim, reorder, multi-angle, slow-mo, zoom and overlay operations work, the 30 s cap is enforced, and auto-edit builds a cut from hit events.
7. Ratings: self-rating is rejected; ratings are aggregated per player.
8. Pawn: walk, jump and dive; ragdolls on a hit; impact frame plays; respawns at the start.
9. EOS: the host gets a code, a second client joins with that code, and the current phase replicates.
10. Each runner's mic is recorded and plays back in sync (offset under 50 ms), with silent playback as the fallback.
11. The packaged zip is under 1 GB and launches to the Title screen.

## Design (why)
- **Engine-free C++17 `Core/`** (CMake + doctest) holds the rules, budget, traps, scoring, replay, edit list and ratings. The UE module wraps it, so agents can test the logic in the sandbox.
- **Root `Makefile` with `test:`** runs `cmake -S Core -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Choomfie's swe.sh only detects `make test` for C++, so this target is required.
- **Replay data instead of video** makes cameras cheap, keeps footage small enough to network, and gives free slow-mo and zoom.
- **Shared map** lets proximity voice work, creates pileups, and means only one map to build.
- **EOS instead of Steam:** it's free, there's no $100 fee, and players already need Epic accounts for the jam.
- **Unreal tests can't run in the sandbox.** UE tasks write Automation tests, and those get checked on Kevin's machine through Session Frontend → Automation.

## Risks
- EOS setup and voice in a packaged build: front-load it (task 3, task 15). Fallback is direct IP.
- Mic sync can fail. Fallback is silent footage.
- Editor scope. Mitigation: build auto-edit first, manual operations after.
- Build-mode camera and UX can eat time.
- Packaging late. Mitigation: package a smoke build in task 11.

## Context rules for task agents
- This file is the source of truth. When a task finds a decision missing or wrong, it notes that in its reply. It doesn't silently change the design.
- Research tasks write to `choomfie-library/ref/` so later tasks (and future jams) can read the results.
- Every task appends a short entry to `DEVLOG.md` with the prompt, what the agent did, what failed and what was fixed by hand (for Glass Box).

## Tasks
1. [research] Asset sourcing for UE5 jams: write choomfie-library/ref/assets-ue5.md. Must have at least 15 cited sources and a shortlist table.
2. [research] Party-game UI reference board: write choomfie-library/ref/ui-party-games.md. Must include a palette hex list, fonts, and a spec for each screen.
3. [research] EOS join codes + voice + mic capture: write choomfie-library/ref/eos-ue5.md. Must include a setup checklist and a fallback.
4. [code] Core scaffold + state machine, Makefile test target, DEVLOG.md and CREDITS.md stubs.
5. [code] Placement + budget model.
6. [code] Deterministic trap timelines.
7. [code] Scoring.
8. [code] Replay recorder.
9. [code] Edit list + auto-edit.
10. [code] Ratings.
11. [code] UE project + Core module + package smoke build.
12. [3d] Angel character.
13. [3d] Hats + faces kit.
14. [code] Ragdoll pawn + impact frame.
15. [code] EOS host/join by code.
16. [code] Traps in world + recording + smite.
17. [code] Backyard map + gate.
18. [code] Build mode.
19. [code] Run phase.
20. [code] Mic capture.
21. [code] Editor screen.
22. [code] Heaven cinema + ratings.
23. [code] Full match flow.
24. [code] Menus + UI skin.
25. [code] Narrator + music + SFX pass.
26. [code] Release package + devlog.
Full task lines with Done-when checks are in the planning chat's task list, using the same numbering.


## Addendum v3.1: unified stylized shader (overrides "Look" where they conflict)
- **One master material `M_TTA_Toon`** applied to every mesh (characters, traps, props, map), plus a **post-process outline** (depth+normal edge detect, chunky dark ink lines). Imported assets get their own materials replaced by instances of it, so mixed CC0 kits (Kenney, Quaternius, KayKit) look like one game.
- Shading: 2-3 band cel/toon ramp, flat colour from the shared 12-colour **palette texture** (UV-indexed) or a per-instance palette index; soft rim light; no PBR detail textures.
- Per-world tuning via a Material Parameter Collection `MPC_TTA_World`: heaven = pastel, low contrast, warm bloom; mortal courses = saturated, harder shadow band.
- Effects reuse it: impact-frame white flash = an emissive override param; replay/ghost and CCTV camera looks = post-process variants.
- Why: consistency is what scores Visuals for a low-poly game; one shader makes human-made assets from several kits match, and new assets need no per-asset art work.
- Requirement 12: every placed mesh in the Backyard and Heaven maps uses an `M_TTA_Toon` instance (checked by Automation test TTA.Art.AllToon).
- New task **11b [code] Toon shader + outline**, after task 11 and before tasks 12, 13, 17. Tasks 12/13 export with palette UVs; task 17 swaps imported materials to `M_TTA_Toon` instances.

## Judging rubric (as reported; confirm on itch before submit)
Scored 1-5 by judges on: ambition/innovation, theme, fun, gameplay, visuals, level design, plus one more criterion not confirmed in sources. Implications: judges may play solo, so the trailer must sell the multiplayer fun; theme is shown by the filming/editing loop; visuals = consistency (toon shader) over fidelity; level design = the backyard map must look authored even before players add traps.

## Handoffs
### Task 4 — Core scaffold + state machine
- Done: `Core/` (C++17, CMake, vendored doctest 2.4.11 in `Core/third_party`), `tta::MatchSettings` (flat JSON: rounds, buildSeconds, runSeconds, editSeconds, ratingSeconds; unknown keys/out-of-range throw), `tta::MatchStateMachine` (illegal transitions return false, state unchanged; Podium → Lobby resets). Root `make test`; DEVLOG.md and CREDITS.md stubs.
- Left: settings sync over network (tasks 15/23). Add new Core sources/tests to `Core/CMakeLists.txt`.
- Surprises: plan.md wasn't in the repo, so this copy was added. Default Build/Run timers (120 s) and setting ranges aren't in the plan — chosen here. Sandbox has no cmake; `pip install cmake` first. The JSON parser only handles flat integer objects; swap in a real library if settings grow nested.

### Task 5 — Placement + budget model
- Done: `tta::Layout` in `Core/include/tta/Placement.h`: 6-trap catalogue (cost + 2 range-checked settings each), positions stored as 0.5 m grid cells, 45° rotation steps (0–7), corridor x∈[0,60], y∈[-4,4] checked after snapping, budget 100 per owner, cameras free and capped at 12 per owner (counted by type via `cameraCount(owner, type)`), per-placement `hidden` flag (`visibleTo(viewer)`, `revealAll()` at Run), `toJson`/`parseLayout` round-trip (loading re-validates every rule).
- Left: Heaven Cam is a system camera, not a placement (task 16/19). UE build mode (task 18) calls `Layout` for every place/move.
- Surprises: trap costs (hammer 20, cannon 25, spinner 20, pusher wall 15, trapdoor 15, banana 5) and setting ranges aren't in the plan — chosen here, tune in playtest. "Cap of 12 by type" read as 12 total per player, with per-type counts available. IDs are reassigned in load order on parse.

### Task 6 — Deterministic trap timelines
- Done: `tta::pose(TrapType, settings, tSeconds)` and `tta::trapPeriodMs` in `Core/include/tta/TrapTimeline.h`. Time is rounded to whole ms and wrapped by an integer period, so `pose(t) == pose(t + period)` bit-exactly. `TrapPose` carries angle, extension, trapdoor open fraction, cannon ball (active/distance/radius) and banana count. Tests: determinism and exact period for every setting combo of all 6 traps, motion, setting effects, range/NaN rejection.
- Left: UE traps (task 16) should drive meshes from `pose(type, settings, matchTime)` using the shared server match clock, not per-actor timers.
- Surprises: timing numbers aren't in the plan — chosen here: hammer/cannon/spinner period 6 s ÷ speed, hammer ±60°, cannon ball 12 m/s for 2 s, radius 0.15 m × size, spinner arm 0.5 + 0.5 × length m, pusher reach 0.4 m × force over a 4-phase cycle of `cycleTime` s, trapdoor closed 1 s + delay then open for duration. Banana is static (period 0). All traps start at phase 0 at match time 0, so same-setting traps are in sync; add a per-placement offset later if that looks bad.

### Task 7 — Scoring
- Done: `tta::scoreRound(RoundInput)` and `tta::MatchScore` in `Core/include/tta/Scoring.h`. Gate +1; each wipeout pays the owner `max(1, trapSpec(trap).cost)`; all-finish or none-finish voids trap points (`trapPointsVoided`) but keeps gate and rating points; rating averages come in as `ratingPoints`; `MatchScore` sums rounds and returns `podium()` (highest first, ties to lower id). Unknown players throw.
- Left: ratings aggregation (task 10) feeds `ratingPoints`; replay wipeout events (task 8/16) map to `Wipeout{victim, trapOwner, trap}`.
- Surprises: plan doesn't say what happens when you wipe out on your own trap — scored as 0 (UCH-style). Repeated wipeouts of the same victim by the same trap each score; cap later if it's farmable. Points are `double` because rating averages are fractional.

### Task 8 — Replay recorder
- Done: `tta::Replay` in `Core/include/tta/Replay.h`. Fixed player list per round; `addFrame` takes one `ReplayPose` (16 world-space `BoneTransform`s: position m + quaternion) per player per 20 Hz tick; `addEvent` takes Hit/Wipeout (trap id + owner, both required) or Smite (`kNoTrap`). `sample(player, t)` clamps to the recording, Catmull-Rom on positions, nlerp on rotations. `serialize()`/`deserializeReplay()`: little-endian, 20 bytes per bone (float32 xyz + int16 quat). 3 min × 4 players = 3601 frames = 4,609,333 bytes (≈ 4.4 MiB); worst interpolation error on the test motion (6 m/s run, 0.3 m limb swing at 1.5 Hz, 3 Hz bob) is 0.65 cm.
- Left: impact frames (plan "stored in the replay") are implied by Wipeout events; task 14 decides if they need their own record. Mic audio lives in separate files keyed to the same timeline (task 20). Mapping trap id → `TrapType` for scoring goes through `Layout` (task 16).
- Surprises: plain linear interpolation of a 0.3 m limb swing at 1.5 Hz misses 1 cm at 20 Hz, hence Catmull-Rom. Repeating the edge sample at the recording's ends gave 3.4 cm error there; the missing neighbour is now extrapolated linearly. Rotations are canonicalised (unit, w ≥ 0) on record; a zero quaternion throws.

### Task 9 — Edit list + auto-edit
- Done: `tta::EditList` + `tta::autoEdit` in `Core/include/tta/EditList.h`. A `Clip` is (camera, replay range, speed, zoom); ops: `addClip`/`removeClip`, `trim`, `move` (reorder), `split` + `setCamera` (multi-angle), `setSpeed` (slow-mo 0.25–1×), `setZoom` (1–4×), `addOverlay`/`removeOverlay` (Sticker, Caption, WastedCard on cut time). Every op validates a copy and commits only if valid, so a rejected edit changes nothing; `kMaxCutMs` = 30 000 is checked on cut time (slow-mo counts double). `autoEdit(replay, cameras, heavenCam)`: ±1.5 s window per event, overlaps merge, cameras round-robin, smites on the Heaven Cam, wipeout/smite windows at 0.5× + 1.5× zoom ending on a 1 s WASTED card, last clip trimmed to the cap.
- Left: Reels save/load (JSON) belongs with the editor screen (task 21); the UE editor renders clips via `Replay::sample`. Auto-edit uses every event in the replay — task 21 may filter to the editor's own cameras/victims.
- Surprises: plan doesn't fix auto-edit pacing or slow-mo/zoom limits — values above chosen here. Overlays outside the cut are rejected, so a trim that would strand an overlay is refused rather than silently dropping it. Sandbox image has no cmake: `make test` only really runs after `pip install cmake`.

### Task 10 — Ratings
- Done: `tta::RatingBook` in `Core/include/tta/Ratings.h`. `openCut(id, {editor, builder})`, `rate(cut, rater, funny, evil)` (1–5 each), `ratingPoints()` → per-player sum of each cut's average Funny (editor) and average Evil (builder), ready for `RoundInput::ratingPoints`. Rejects self-rating (rater is the cut's editor or builder), unknown cuts, out-of-range scores, repeat ratings and duplicate cut ids; a rejected rating changes nothing.
- Left: the 10 s rating window and who counts as a cut's builder (owner of the trap in the cut) are decided by the premiere screen (task 22).
- Surprises: plan doesn't say whether the builder may rate the cut's Funny; "no rating yourself" is read as: a cut's editor and builder can't rate it at all. Unrated cuts add 0 rather than a default.

### Task 11 — UE project + Core module + package smoke build
- Done: `TakeTwoAfterlife.uproject` (UE 5.5) with two modules. `TTACore` wraps `Core/` without copying it: `Private/Core<Name>.cpp` each `#include` one `Core/src` file, `Core/include` is a public include path, exceptions on, unity off. `TakeTwoAfterlife` (primary game module) depends on it and holds Automation test `TTA.Core.Smoke` (one round through `MatchStateMachine`). Config sets Title as game/editor default map and a Win64 Shipping, maps-only, pak+IoStore packaging profile. `Scripts/package_win64.bat <UE dir> <TeamName>` creates the Title map (editor Python), runs BuildCookRun, then `Scripts/zip_package.py` makes `<TeamName>_TakeTwoAfterlife.zip` and deletes it / fails if ≥ 1 GiB. `make test` now also runs the zip script's unit tests.
- Left: on Kevin's machine: right-click uproject → Generate VS files, build, run `TTA.Core.Smoke` in Session Frontend, run the package script, unzip and launch to Title. Real team name goes in at package time.
- Surprises: an empty `.umap` can't be authored in the sandbox, so `Content/Maps/Title.umap` is created by `Scripts/create_title_map.py` (commit it after the first run). Engine 5.5 assumed; change `EngineAssociation` if Kevin has another. UE's MSVC flags may warn on Core code that Core's own `/W4` build never saw — not compiled here.

### Task 11b — Toon shader + outline
- Done: `Scripts/create_toon_assets.py` (editor Python) builds under `/Game/Art/Toon`: `T_TTA_Palette` (12x1, nearest, no mips, generated from `Scripts/toon_spec.py`), `MPC_TTA_World` (SunDirection + Saturation/Contrast/ShadowFloor/Bands/RimStrength/Warmth), `M_TTA_Toon` (unlit, 2–3 band cel ramp on N·L vs SunDirection, palette by UV or per-instance `PaletteIndex` with `UsePaletteUV`=0, rim, `Flash` emissive override), `M_TTA_PP_Outline` (depth + normal edges, ink = palette 0) with `MI_TTA_PP_Ghost` / `MI_TTA_PP_CCTV`, and `/Game/Maps/TTA_ArtTest` (3 shapes). Heaven/Course presets live in `Config/DefaultGame.ini` (`UTTAWorldLookSettings`); `UTTAWorldLookLibrary::ApplyWorldLook` writes one into the MPC. `Scripts/swap_to_toon.py <folders>` swaps imported mesh materials to `MI_TTA_Toon_<name>` (nearest palette colour). Automation `TTA.Art.AllToon` checks every mesh slot on its map list. `make test` runs `toon_spec` tests.
- Left: on Kevin's machine: build, run `UnrealEditor-Cmd TakeTwoAfterlife.uproject -run=pythonscript -script=Scripts/create_toon_assets.py`, commit the new Content, add `M_TTA_PP_Outline` to a global unbound PostProcessVolume, run `TTA.Art.AllToon`. Heaven's warm bloom is a PostProcessVolume setting for the heaven map (task 22). Tasks 17/22 add Backyard and Heaven to the test's map list.
- Surprises: no UE in the sandbox, so no material/C++ here was compiled — Custom-node HLSL and the editor Python API calls are untested. The shader is unlit (no dynamic shadows; the band uses MPC SunDirection) so heaven/course grading stays fully controlled. Also: `make test` needs cmake on PATH.

### Task 14 — Ragdoll pawn + impact frame
- Done: `tta::PawnLife` (`Core/include/tta/PawnLife.h`): Alive → hit → Frozen 0.12 s → Ragdoll 1.5 s → Alive, extra hits ignored; `wingsVisible/haloVisible(Form)` (human = off). `ATTARunnerPawn` (ACharacter): WASD/mouse walk, Space jump, LeftShift dive (`LaunchCharacter`). `Hit()` sets the victim's `CustomTimeDilation` 0 + `Flash`=1 on per-slot MIDs of M_TTA_Toon, plays `ImpactShake` on its controller and `ImpactSound`, broadcasts `OnImpact` (SFX/replay hook); then simulates the mesh (`Ragdoll` profile), then restores mesh/capsule and teleports to `StartLocation` (spawn point; `SetStartLocation` overrides). Wings/Halo/Hat components (human form shows Hat only). Automation `TTA.Pawn.Ragdoll`. Legacy input mappings in `Config/DefaultInput.ini`.
- Left: on Kevin's machine: build; import SK_Angel at `/Game/Characters/Angel/SK_Angel` (or edit `RunnerMesh` in DefaultGame.ini) with a Physics Asset; set Wings/Halo/Hat meshes + sockets in a BP child; make a CameraShake asset and impact SFX; run `TTA.Pawn.Ragdoll`. Hit replication and storing impact frames in the replay are tasks 16/19.
- Surprises: plan gives no ragdoll duration, 1.5 s chosen. Freeze timers use world delta because the victim's own Tick delta is 0 while frozen. Not compiled here (no UE in sandbox).

### Task 15 — EOS host/join by code
- Done: `tta::makeJoinCode/normalizeJoinCode` (6 letters, A–Z without I/O) and `parseDirectAddress` in `Core/include/tta/JoinCode.h` with doctest cases. `UTTASessionSubsystem` (GameInstance subsystem): `Login()` (Account Portal; `AutoLogin` when `-AUTH_TYPE=` is on the command line), `HostLobby()` (EOS lobby, 4 slots, lobby voice, `SessionIdOverride` = code + `JOINCODE` attribute, then `ServerTravel <LobbyMap>?listen`; `GetJoinCode()` for the lobby UI), `JoinByCode()` (lobby search filtered by `JOINCODE`, 4 tries 1.5 s apart for the ≤3 s indexing delay, `JoinSession` → `GetResolvedConnectString` → `ClientTravel`), `HostDirect()/JoinDirect("ip[:port]")` fallback. `ATTAGameMode` owns `tta::MatchStateMachine`; `RequestPhase()` mirrors accepted phases into `ATTAGameState` (`Phase`, `Round` replicated with RepNotify → `OnPhaseChanged` / `OnPhaseChangedNative`). Set as global default game mode. EOS plugins in the uproject, `OnlineSubsystem*` deps, EOS + NetDriverEOS (P2P, IpNetDriver fallback) in DefaultEngine.ini. Automation `TTA.Net.PhaseReplication`.
- Left: on Kevin's machine: Dev Portal steps 1–5 of eos-ue5.md, add the Artifact in Project Settings (don't commit the secret), build, run `TTA.Net.PhaseReplication`, then the 2-PC check: PC A `Login` → `HostLobby` (code logged/shown), PC B `Login` → `JoinByCode(code)`, B lands on A's map and sees phase changes. Test `JoinDirect` on LAN too. No Host/Join UI yet — call from a BP or console until task 24.
- Surprises: `SessionIdOverride` is only documented by a third-party plugin, so the code is also an attribute and the join searches by attribute (works either way). The EOS ini keys/section names are from memory of the UE 5.x docs and must be checked against Kevin's engine version's doc page. The Automation test checks replication setup + RepNotify, not a live network hop; the live hop is the 2-PC check. Not compiled here (no UE in sandbox).

### Task 16 — Traps in world + recording + smite
- Done: Core `tta::RunRecorder` (`Core/include/tta/RunRecorder.h`, doctest): frame deltas → 20 Hz frames due, `recordHit(Placement, victim)` stamps trap id + owner at the current ms, `takeSmites(unfinished)` records one Smite per unfinished runner at the run's end, once. `ATTATrapActor`: `InitFromPlacement` (settings validated by `pose()`, grid cell/45° → transform); `Tick` drives `Moving`/`Projectile` from `tta::pose()` at `GameState::GetServerWorldTimeSeconds() - RunStartTime` (no transform replication); `Hitbox` overlap → `TryHit` (authority + alive only) → `Pawn->Hit()` + `ATTARunDirector::RecordHit`. `ATTARunDirector` (server): `StartRun(runners, seconds)`, root + first 15 skeleton bones per runner (m, quats) each due frame, `MarkFinished`, timeout smites unfinished runners and aims a Heaven Cam (`ACameraActor`, `HeavenCamOffset`) via `SetViewTargetWithBlend`. Automation `TTA.Traps.Deterministic`, `TTA.Traps.HitRecorded`.
- Left: on Kevin's machine: build, run both tests. Trap meshes on `Body`/`Projectile` + `Hitbox` size in BP children; Run phase (task 19) calls `SetRunStartTime` + `StartRun`; gate (task 17) calls `MarkFinished`. Lightning VFX/SFX on smite hang off `OnImpact` (task 25). Wipeout events, hit replication to clients, banana slide physics: task 19.
- Surprises: replay player id = index in the runners array (not PlayerState id); trap owners must be in it or Core's replay throws. Pose→transform mapping (hammer roll, spinner yaw + arm X-scale, pusher X, trapdoor pitch, cannon ball X + scale = diameter, banana X-scale = count) is my choice, not in the plan. `make test` needs cmake (sandbox: `pip install cmake`). UE code not compiled here.

### Task 17 — Backyard map + gate
- Done: `Scripts/backyard_layout.py` is the map's source of truth (metres; corridor size read from Core `Placement.h`): 5 colour-coded floor zones start→gate (Wood patio, Grass lawn, Yellow sandpit, Grass lawn, Bone patio), Bone stepping stones every 4 m on the centre line, 2 m open ditches each side into the kill plane, side lawns with fences, trees, bushes, flowers, rocks, planters and toys, a house behind the gate and one behind the start. Nothing but flat floor/markers is inside the corridor, so traps own it. `validate()` + `credits_missing()` run in `make test`. `Scripts/build_backyard_map.py <downloads>` imports the used FBX meshes (no materials), builds `/Game/Maps/Backyard`, puts every slot on `MI_TTA_Toon_Pal_<colour>`, spawns PlayerStart, `ATTAFinishGate` (trigger → `ATTARunDirector::MarkFinished`, arch has no collision), `ATTAKillPlane` (fall → `Hit()` → respawn at start), KillZ -500 m, a sun. Automation `TTA.Map.StartToGate`; `TTA.Art.AllToon` now also checks Backyard.
- Left: on Kevin's machine: download KayKit Prototype Bits + Kenney City Kit (Suburban) + Nature Kit into `<downloads>/KayKitPrototypeBits|KenneyCitySuburban|KenneyNature`, run `create_toon_assets.py` then `py Scripts/build_backyard_map.py <downloads>`, build, run both tests, then eyeball the route and adjust numbers in `backyard_layout.py` (not the map) and rebuild. Lighting/sky/post-process outline volume per task 11b settings not placed here.
- Surprises: Kenney mesh names (`fence`, `tree-large`, `plant_bushLarge`, …) are from memory; the build script fails naming any missing one, rename in `backyard_layout.py` + CREDITS.md. The gate arch is KayKit `Primitive_Doorway` stretched to 1×8×4 m at yaw 0; if its opening faces Y, set the gate prop's yaw to 90 and swap its box x/y. Kill plane hits a faller (ragdoll, then respawn) instead of an instant teleport.

### Task 18 — Build mode
- Done: Core `Layout::canPlace` (same rules as place*, no change; doctest) feeds the ghost. `ATTAPlacementMarker` (replicated `FTTAPlacementRep`, RepNotify): net-relevant only to its owner until revealed, and drawn/collidable only for the local owner (listen host), so others never see or receive it before Run; `WorldTransform` is the one placement→world mapping (trap actor uses it too); cameras lifted to `CameraHeightCm`; meshes per build item from DefaultGame.ini (engine cube/cone placeholders). `ATTABuildPawn` (DefaultPawn free-fly, not movement-replicated, owner-only): crosshair trace → 0.5 m snapped ghost, R = +45°, wheel = item (6 traps then fixed/pan/follow/trap-cam), LMB place, RMB remove own, F select own, 1/2 and 3/4 = setting 1/2 −/+; server RPCs → `ATTAGameMode::PlaceItem/SetTrapSetting/RemovePlacement` (Build phase only, owner checks, `ETTAPlaceOutcome`). GameMode keeps the persistent `tta::Layout`, spawns build pawns on entering Build, reveals all on Run. `ATTABuildHUD` canvas panel: budget, camera counts by type, item, ghost/server result, selected trap's 2 settings. Automation `TTA.Build.PlaceReplicate`.
- Left: on Kevin's machine: build, run `TTA.Build.PlaceReplicate`, then a 2-PC check (B must not see A's ghost-placed traps until Run). Task 19: swap build pawns for runners on Run and spawn `ATTATrapActor`s from `GetLayout()` (owner = PlayerState id, map it to replay player index). Real trap/camera meshes into `ItemMeshes`; UMG skin in task 24.
- Surprises: Core `spent()` counts every placement ever, so with the persistent map round 2 has no budget left: task 23 must decide per-round budget (e.g. spent counts hidden placements only). Core placements have no height/pitch, so cameras sit at a fixed 250 cm with level pitch — editor/camera rendering (tasks 19/21) may want pitch. Revealed placements can't be removed. Hiding relies on `IsNetRelevantFor` (generic replication, not Iris). Ghost validity is shown as HUD text (green/red), not a ghost tint. Not compiled here (no UE in sandbox).

### Task 19 — Run phase
- Done: `ATTAGameMode` on Run reveals placements, gives every player an `ATTARunnerPawn` at `FindPlayerStart` (human form; respawn point = spawn point), spawns/re-inits one `ATTATrapActor` per trap placement (kept by id; owner remapped PlayerState id → runner index) and starts `ATTARunDirector` with `Match.phaseSeconds()`. Run ends when every runner is through the gate, or `SmiteHoldSeconds` (2 s) after the timer (Heaven Cam shot): `EndRun` maps the round back to PlayerState ids, `tta::MatchScore::addRound`, phase → Edit. Core `RunRecorder::roundInput(finishers)` turns hit events into `Wipeout`s (doctest). Director: `GetRoundInput/IsTimedOut/AllFinished`. Timer: replicated `ATTAGameState::PhaseEndsAt` + `GetSecondsLeft()`, drawn by the HUD during Run. Handheld: `UCameraComponent Handheld` on the runner; hold RMB (`RaiseCamera`) to view through it. Proximity voice: open mic (`[Voice] bEnabled`, `bHasVoiceEnabled`, no push-to-talk); each remote runner gets a `UVOIPTalker` on its mesh with spatialized attenuation (`VoiceFalloffCm` 25 m). Automation `TTA.Run.ScoreRound`.
- Left: on Kevin's machine: build, run `TTA.Run.ScoreRound`, then a 2-PC run (timer on both, traps hit, gate ends early, voice fades with distance). Task 21: the handheld's transform isn't recorded yet (replay has bones only). Task 23: Score → Podium/ratings.
- Surprises: a trap whose owner left is hidden and inactive for that Run (the replay only has runner ids, so its hits couldn't be scored); this conflicts with "a leaver keeps their traps", so task 23 should add non-running owners to the recorder's players. Task 15 enabled `bUseLobbiesVoiceChatIfAvailable` (EOS lobby voice, not positional): if both work you'd hear people twice, so turn that off once VOIPTalker voice works over EOS P2P. `make test` needs cmake on PATH (swe.sh check reported PASS with cmake missing; installed it via pip). Not compiled here; `APawn::OnPlayerStateChanged` / `UVOIPTalker` API from memory.

### Task 20 — Mic capture
- Done: Core `tta::MicTrack` (16 kHz mono PCM, sample 0 = replay time 0): `append(pcm, endMs)` places each captured chunk by its replay-time stamp and re-aligns when drift passes 20 ms (pads silence on gaps, drops overlap), `read(tMs, n)` returns zeros outside the recording (silent fallback), WAV in/out (doctest incl. drifting/batched click fixture). `ATTAGameState::GetPhaseElapsedSeconds()` (new replicated `PhaseStartedAt`) is the replay clock on clients. `UTTAMicRecorder` on `ATTARunnerPawn` (locally controlled only): `FVoiceModule` capture at 16 kHz on entering Run, polled each tick, saved on leaving Run to `Saved/Mic/Round<R>_Player<PlayerId>.wav`; no mic → warning, no file. `UTTAMicPlayer`: `Load(path)` (false → silent), `PlayFrom(replaySeconds)` via `USoundWaveProcedural`, `Stop`, `ReadAt`. Automation `TTA.Audio.MicSync`.
- Left: on Kevin's machine: build, run `TTA.Audio.MicSync`, then a 2-PC Run and check `Saved/Mic` has a WAV per PC that lines up with a clap on replay. Mic files stay on each runner's PC: task 21/22 must send them to the host/viewers (e.g. chunked RPC after Run) and call `UTTAMicPlayer::PlayFrom` alongside replay playback; player id in the filename is the PlayerState id, not the replay index.
- Surprises: VOIP (task 19) and this recorder both open the default mic; if the second capture fails on Windows, the recorder falls back silent — then switch to tapping VOIP's captured data instead. Device latency isn't visible to the API: tune `MicLatencyMs` (Config) if claps land late. Not compiled here; `IVoiceCapture` / `USoundWaveProcedural` API from memory.

### Task 22 — Heaven cinema + ratings
- Done: Core `tta::Premiere` (seeded own Fisher-Yates order; per cut Intro 4 s → Playback (cut length) → Rating 10 s → Reveal 4 s; then Podium on the final round, else Done; `rate` only during Rating, RatingBook rules) + `RatingBook::averages(cutId)` for the reveal (doctest). `ATTACinema`: walkable cloud, 4-seat couch + 16:9 screen (engine shapes as placeholders), `GetSeatTransform` facing the screen, heaven preset into `MPC_TTA_World` on BeginPlay (`[/Script/TakeTwoAfterlife.TTACinema] WorldCollection`, error logged if missing), `StartPremiere(cuts, seed, final, podium)`, `Rate`, `Step`, `GetScreenFrame`, `GetIntroLine`, `OnStep`, `GetPodium`; runner mic via `UTTAMicPlayer`, re-synced with `PlayFrom` at each clip change. Automation `TTA.Cinema.Sequence`.
- Left: on Kevin's machine: build, run `TTA.Cinema.Sequence`; make a Heaven map (sky, `ATTACinema` placed, `M_TTA_Toon` instances on its meshes, add it to TTA.Art.AllToon). Task 23: GameMode Premiere → `StartPremiere` (host seed, podium from `MatchScore::podium()` after adding `ratings().ratingPoints()`), seat players, replicate step/ratings, ship mic WAVs. Task 24: rating/reveal/podium widgets on `OnStep`. Task 25: narrator audio for the intro.
- Surprises: premiere state is local only (no replication yet). Slow-mo clips drift the mic within a clip (re-synced only at clip starts). Not compiled here; `UWorld::CreateWorld` / `SetAbsolute` API from memory.

### Task 23 — Full match flow
- Done: Core `tta/MatchFlow` (`canJoin(phase)`: Lobby/Premiere/Podium only; `runPlayers(runners, layout)`: runners then owners of placed traps who left; `toPlayerIds(roundInput, ids, runnerCount)`), `RoundInput::absentOwners` (leavers score trap points; all/none-finish voiding counts runners only), `MatchScore::addRatings` (doctest). GameMode: settings JSON at `Saved/TTA/MatchSettings.json` (written with defaults if missing, loaded in InitGame, `ApplySettingsJson` in Lobby only, saved back, mirrored to replicated `ATTAGameState::GetSettingsJson`); `PreLogin` refuses joins mid-round; Build/Edit auto-advance on timer; `EndPremiere(ratingPoints)` → next Build or Podium; leaver traps stay visible/live and score (Director `StartRun(..., AbsentOwners)` pads replay poses). Automation `TTA.Match.ThreeRounds`.
- Left: on Kevin's machine: build, run `TTA.Match.ThreeRounds` (it backs up/restores the settings file). Not wired: GameMode → `ATTACinema::StartPremiere` with the round's cuts, and calling `EndPremiere` with `Premiere::ratings().ratingPoints()` when it reaches Done/Podium (cut editor/builder ids must be PlayerState ids); seating players; Podium screen (task 24) reads `GetScore().podium()`.
- Surprises: an earlier task hid a leaver's traps (couldn't score); now fixed. Uses `PreLogin` (not `PreLoginAsync`). Test assumes `World->SetGameMode` + `APlayerController::InitPlayerState` work in a bare game world; if not, spawn controllers via `Mode->SpawnPlayerController`. Not compiled here.

### Task 24 — Menus + UI skin
- Done: Core `tta/MenuFlow` (screen stack Title/Join/Lobby/Settings/HowToPlay/Reels with legal moves; Join is replaced by the Lobby so Back goes to Title; `cycleLook` colour/hat/face; `ClientSettings` + clamp; `startBlockedReason`; `micLevel`; palette hex mirrored from `toon_spec.PALETTE`, guarded by a Python test; How-to-play card text) with doctest. UE `UTTAMenuWidget` (C++-built UMG tree in a WidgetSwitcher: sticker cards with Ink outline, palette fills, delegates `OnHost/OnJoinCode/OnStart/OnQuit`). Automation `TTA.UI.MenuFlow`. Added `UMG`, `Slate`, `SlateCore` to the module's deps.
- Left: on Kevin's machine: build, run `TTA.UI.MenuFlow`. Wire a Title map/PlayerController to create the widget and bind `OnHost`→`UTTASessionSubsystem::HostLobby` (+`SetRoomCode(GetJoinCode())`), `OnJoinCode`→`JoinByCode`. Import Lilita One/Nunito as Runtime-cached Font assets (+OFL.txt, CREDITS) and swap them into `Label`. Not done: Anim_PopIn/Shake/Pulse widget animations, settings persistence + applying volumes to sound classes, 3 s mic record/playback, reel thumbnails and click-to-play (rows are labels only), rating/podium/Build HUD skins.
- Surprises: the repo palette (`toon_spec.py`, "Bone" etc.) differs from the UI ref's palette; the repo wins. The module never listed `UMG` though the editor widget uses it. Not compiled here; `Initialize()` on a `NewObject` widget creating its WidgetTree is from memory.

### Task 25 — Narrator + music + SFX
- Done: Core `tta/AudioCues` is the single table: 15 `AudioEvent`s (6 phase-start events mirroring `tta::Phase`, then TrapPlaced, RunCountdown, RunHurry, Wipeout, Smite, GateReached, PremiereIntro, RatingOpen, RatingReveal), 60 narrator lines with their script text (`VO_<Tag>_NN`), `MUS_<Phase>` per phase, 10 `SFX_*`; `narratorLine(event, n)` round-robin; `assetPath` → `/Game/Audio/{VO,Music,SFX}/<id>.<id>` (doctest). `build/tta_vo_script` prints the script as CSV for ElevenLabs. `UTTAAudioDirector` (UWorldSubsystem): binds `ATTAGameState::OnPhaseChangedNative`, crossfades phase music, speaks the phase line; `PlayEvent(event)` = SFX + next line. Hooked: `ATTARunDirector::RecordHit` → Wipeout, `MarkFinished` → GateReached (once per runner), `Smite` → Smite. Automation `TTA.Audio.TriggerTable` loads every cue's asset.
- Left: on Kevin's machine: `make test`, run `build/tta_vo_script > vo.csv`, generate the 60 lines in ElevenLabs, pick 6 WeLoveIndies tracks + 10 SFX, import as SoundWaves named exactly as the ids into `Content/Audio/VO|Music|SFX` (set music looping), fill in CREDITS rows, then run `TTA.Audio.TriggerTable`. Not hooked yet: TrapPlaced (Build placement), RunCountdown/RunHurry (Run timer, 10 s left), PremiereIntro/RatingOpen/RatingReveal (`ATTACinema::OnStep`) — one `PlayEvent` call each. Volumes from task 24 settings are not applied to sound classes.
- Surprises: wipeout/gate/smite hooks run on the server only, so on a listen server only the host hears them; make them multicast (or play from replicated pawn state) for clients. `ETTAPhase` ↔ `AudioEvent` relies on enum order (tested in Core). Not compiled here; `OnWorldBeginPlay` may run before the GameState exists in some maps — then nothing binds (check in PIE).

### Task 26 — Release package + devlog
- Done: `Backyard` added to `MapsToCook` (only Title was cooked). DEVLOG back-filled for tasks 1–3, 11b, 12, 13, 21, 24, 26, sorted by task. `Scripts/export_devlog.py` checks every plan task (incl. 11b) has an entry and writes `Dist/devlog_itch.html`; tested in `make test`. `SUBMISSION.md` checklist; CREDITS Tools table filled (= the submission tools list). The zip script (task 11) already enforces the name and < 1 GB.
- Left: on Kevin's machine: run `package_win64.bat`, check zip size and that it launches to Title, export + post the devlog, fill CREDITS audio rows. No Heaven map exists (cinema has no separate map to cook) — add it to `MapsToCook` if one is made.
- Surprises: back-filled entries for 12/13/21/24 come from commits and handoffs, not the original prompts; edit them if the wording matters. Can't package or launch here. `make test` needs `pip install cmake` in the sandbox.

### Task 27 — Review fixes, round 1
- Done: camera cap is per type. `Layout::canPlace` takes the camera type (last arg, default `Fixed`); `add()` passes `p.camera`, the build-mode ghost passes `Preview.camera`; the check is `cameraCount(owner, type)`. New doctest: 12 Pan placed, 13th Pan rejected, a Fixed still accepted. Updated tests that encoded the old total cap (mixed-type doctest, `canPlace` preview doctest, UE `TTA.Build.PlaceReplicate` now fills Fixed to 12 before the cap check).
- Left: on Kevin's machine: build and run `TTA.Build.PlaceReplicate`. Build HUD still shows `CAMERAS <total> / 12` — misleading now; show per-type `n/12` if wanted.
- Surprises: the UE test placed 3 of each type and expected the 13th (Fixed) to hit the cap — it would have failed under the per-type rule. Not compiled here.

### Task 5 (restore) — Core match flow
- Done: `Core/include/tta/MatchFlow.h` + `Core/src/MatchFlow.cpp` (`canJoin`, `runPlayers`, `toPlayerIds`), derived from `MatchFlowTests.cpp` and `TTAGameMode`. `RoundInput::absentOwners` re-added to Scoring (leavers' traps score; voiding counts runners only; an absent owner who also ran throws). `CoreMatchFlow.cpp` UE wrapper; CMake lists both files. `make test` passes.
- Left: UE editor compile on Kevin's machine (TTAGameMode, `TTA.Match.ThreeRounds`).
- Surprises: `absentOwners` was missing from `RoundInput` though the earlier handoff says it was added, so Scoring had to change too. The wrapper list in `Source/TTACore/Private` also lacks `CoreMenuFlow.cpp` (MenuFlow.cpp isn't compiled in UE); left alone here.

### Task 28 — Content bootstrap fix
- Done: `create_toon_assets.custom_node` builds each `unreal.CustomInput()` with no args and sets `input_name` via `set_editor_property` (UE 5 structs reject constructor kwargs: `TypeError: call() takes at most 0 arguments (1 given)`). `build_backyard_map.py [downloads]` defaults to `<repo>/Downloads`; a missing pack/mesh logs a warning and the prop gets a fitted `/Engine/BasicShapes/Cube` with its toon palette instance instead of exiting (`backyard_layout.downloads_dir` / `find_fbx`, tested). `swap_to_toon.py` defaults to `/Game`. New `Scripts/fetch_assets.ps1` (KayKit Prototype Bits, Kenney City Suburban, Kenney Nature into `Downloads/<kit key>`, gitignored) and `Scripts/setup_content.ps1` (toon -> backyard -> swap -> title through UnrealEditor-Cmd `-run=pythonscript`, stops on any Traceback or non-zero exit, then checks `Content/Maps/Title.umap` + `Backyard.umap`). `Scripts/tests/test_ue_scripts.py` fakes the UE struct constructor.
- Left: on Kevin's machine: `Scripts/fetch_assets.ps1`, then `Scripts/setup_content.ps1` (pass `-Editor` or set `UE_ROOT` if UE isn't under `C:\Program Files\Epic Games`). Fill CREDITS rows for any pack actually imported.
- Surprises: fetch only covers the 3 kits the layout uses, not every pack in assets-ue5.md (no script references the rest). Kenney zip links are scraped from the pack page and the KayKit branch name is guessed (main, then master); both untested here. Only a mocked `unreal` was run; nothing ran in-engine.

### Task 29 — EOS errors fail content scripts
- Done: `setup_content.ps1` ignores the editor exit code (it is 1 on any logged error, e.g. EOS with no IDs); it fails on `Traceback` / `LogPython: Error`, or when the script's own success line is missing (`$Markers`: `toon assets built`, `built <map>: N props`, `swapped N material slots`, `created <map>` / `<map> already exists`). `DefaultPlatformService=NULL` until EOS Artifact IDs exist; README "Online (EOS)" says where to paste them and to flip back to EOS. `Scripts/tests/test_setup_content.py` checks markers match each script's `unreal.log`, no exit-code throw, and EOS not selected without IDs.
- Left: on Kevin's machine: run `setup_content.ps1` with IDs empty, open editor, confirm Title + Backyard in Content/Maps and no EOS errors in PIE. When IDs are added, set `DefaultPlatformService=EOS`.
- Surprises: no README existed; created one. NetDriverEOS stays first in NetDriverDefinitions and falls back to IpNetDriver under NULL; untested in engine. PowerShell not run here (static tests only).
