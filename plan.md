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
- **Assets: AI-generated assets are allowed** (e.g. Tripo/Meshy 3D models, ElevenLabs VO) alongside CC0 kits: Kenney, Quaternius, KayKit, Poly Haven, Freesound, free items on Fab. Log every asset in CREDITS.md, including which tool generated it (the jam's tools list must name every AI tool used).

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

### Task 30 — Title map plays black with build HUD
- Cause: `Scripts/create_title_map.py` made an empty Title level with no World Settings GameMode override, so it ran `GlobalDefaultGameMode` (`ATTAGameMode`, `HUDClass = ATTABuildHUD`) with no lights; and nothing at runtime created `UTTAMenuWidget` (only the UI test did).
- Done: new `ATTATitleGameMode` (no pawn, plain `AHUD`) + `ATTATitlePlayerController` (BeginPlay: creates the menu; Host -> `HostLobby` under EOS / `HostDirect` under NULL; Join code -> `JoinByCode`; new Direct IP field + button -> `JoinDirect`; Quit; cursor on, UI-only input, view target = CameraActor tagged `TitleCamera`). `create_title_map.py` deletes and rebuilds Title on every run: GameMode override, DirectionalLight, SkyLight, SkyAtmosphere, ExponentialHeightFog, toon backdrop + ground planes, tagged camera; still logs `created /Game/Maps/Title` (setup_content marker). `build_backyard_map.py` sets Backyard's override to `TTAGameMode` explicitly; global default stays `TTAGameMode`. `UTTASessionSubsystem::LobbyMap` default Title -> Backyard (the host used to listen-travel back to Title). Test: `Scripts/tests/test_title_map.py` (mocked unreal + source checks).
- Left: on Kevin's machine: compile, re-run `setup_content.ps1`, 2-player PIE on Title (Net Mode: Standalone so each window can host/join), Host in Client 0, Direct IP `127.0.0.1:7777` (or the code under EOS) in Client 1; confirm no TTABuildHUD in the Title Outliner.
- Surprises: `delete_asset` fails if Title is open in a running editor (setup uses the commandlet, so fine). After travel there is no lobby UI in Backyard: the widget's Lobby screen lived on the Title controller. Not compiled here.

### Task 31 — Missing audio assets spam errors
- Cause: `UTTAAudioDirector::LoadCue` called `LoadObject` and logged a LogTTAAudio **Error** on every play, with no memory of earlier misses; no audio is imported yet, so every phase change/event re-logged (plus LoadObject's own warning each call).
- Done: Core `tta::MissingAssetLog` (doctest). `LoadCue` checks `FPackageName::DoesPackageExist` first, loads with `LOAD_NoWarn | LOAD_Quiet`, logs one LogTTAAudio **Warning** per missing path per session, then returns nullptr with no lookup. Session = first game-world director `Initialize` to last `Deinitialize` (a whole PIE run; packaged game lifetime). New `Scripts/create_placeholder_audio.py` (+ engine-free `placeholder_audio.py`, tested) imports a 0.25 s silent WAV as a SoundWave at every `/Game/Audio/<VO|Music|SFX>/<id>` in the Core cue table (76; music set to loop), skipping any `Content/Audio/.../<id>.uasset` already there. Last step of `setup_content.ps1` (marker `placeholder audio: N created, M kept`).
- Left: on Kevin's machine: compile, run `setup_content.ps1`, PIE on Title: no LogTTAAudio lines with placeholders; without them, one warning per asset. `TTA.Audio.TriggerTable` now passes on placeholders, so it no longer proves real audio is in: check imports before submit. To replace a placeholder, import the real file with the same name over it.
- Surprises: the director is a world subsystem in every game world, so Title (GameState phase Lobby) triggers MUS_Lobby + VO_Lobby_xx. Not compiled or run in-engine here.

### Submission docs — Nov 13 status
- Done: `SUBMISSION.md` "Status as of Nov 13" (323.6 MiB Shipping zip, exe not launched, red Automation suite, placeholder Backyard art, unmerged branches). DEVLOG entries for tasks 28–31 and the premiere/respawn fix (31b). CREDITS: VO script source filled; TODOs for KayKit/Kenney and audio.
- Left: merge `overnight/cursor` and `mwe4dnfb`, then rewrite the 28–31b DEVLOG lines from their handoffs; launch the exe; fill audio credits.
- Surprises: no task 28–31 work, handoffs or `fetch_assets.ps1` exist on this branch, so those DEVLOG entries are written from the status notes only. Tasks 28–31 aren't in plan.md's task list, so `export_devlog.py` doesn't require them.

### Glass Box public dev log
- Done: `Docs/DEVLOG_PUBLIC.md`, a readable public log from DEVLOG.md, handoffs and the "Fix UE compile + link on Windows" commit: concept, Director's Cut fit, plan v3 timeline table, Core/Unreal split, multi-agent workflow, what broke (duplicate Core sources at link, MSVC includes/C4458, Automation crashes). Screenshot slots are `<!-- TODO(screenshot) -->`. `Scripts/tests/test_devlog_public.py` checks the sections.
- Left: add screenshots; post it to the itch devlog; update the 28–31b and Windows-workflow lines after `overnight/cursor` / `mwe4dnfb` merge.
- Surprises: nothing in the repo records the Grok/Cursor workflow or the exact LNK error text; those paragraphs come from the task brief and the fix commit's diff (TTACore dependency dropped, Core compiled once via CoreLib wrappers). `export_devlog.py` output (`Dist/`) isn't committed, so DEVLOG.md was the source.

### Itch page text
- Done: `Docs/ITCH_PAGE.md` — pitch, short/long description (Director's Cut), controls, how to play (rounds, traps, replay, premiere), tools used, team credits. `Scripts/tests/test_itch_page.py` checks sections and key terms.
- Left: fill TODOs — gamepad/editor/rating/voice controls, team names/roles, team name, confirm no Meshy assets.
- Surprises: keyboard bindings came from Config input mappings; editor/rating/voice inputs aren't there, so those are TODO. Plan forbids AI-generated assets, so Meshy is listed as "confirm none".

### Phase timings config
- Done: `tta::MatchSettings` holds every phase length (rounds 3, Build 120, Run 120, Edit 90, premiere intro 4 / rating 10 / reveal 4 s); `introSeconds`/`revealSeconds` added, `Premiere` takes the settings (the `kIntroMs/kRatingMs/kRevealMs` constants are gone). `parseMatchSettings(json, base)` keeps `base` for missing keys. UE: `UTTAMatchSettings` (UDeveloperSettings) reads `[/Script/TakeTwoAfterlife.TTAMatchSettings]` in DefaultGame.ini (section added with the defaults); `ATTAGameMode::IniSettings()` is the base for the saved host JSON; `ATTACinema::StartPremiere` uses the match's settings. New doctests for custom phase and premiere lengths.
- Left: compile in UE on Windows (new UCLASS, regenerate project files). Tasks 2-4.
- Surprises: an existing `Saved/TTA/MatchSettings.json` (written with all keys) still overrides the ini; delete it after changing ini timings. Bad ini values log an error and fall back to code defaults. `ratingSeconds` already existed but Premiere ignored it.

### Settings: fullscreen + resolution
- Done: Settings screen gets a Display picker (Fullscreen / Windowed Fullscreen / Windowed) and a Resolution picker (`UComboBoxString`) under the sensitivity slider. List comes from `UKismetSystemLibrary::GetSupportedFullscreenResolutions`, cleaned by Core `tta::resolutionChoices` (dedupe, drop 0-size, largest first); current size picked by `tta::resolutionIndex`. Player picks call `UGameUserSettings::SetFullscreenMode` / `SetScreenResolution` + `ApplySettings(false)` (saves GameUserSettings.ini). No scalability presets added. Core doctests cover the list and the `WindowMode` == `EWindowMode` order; `TTA.UI.MenuFlow` checks an out-of-range resolution is refused.
- Left (Windows): build, open Settings, switch each mode and a resolution, restart and confirm they stick. No confirm/revert countdown and no `OnGameUserSettingsVideoRevert` binding yet.
- Surprises: changes apply instantly on pick (no Apply button). Picker selections set by code (`ESelectInfo::Direct`) are ignored so opening the screen never re-applies settings. If the platform reports no resolutions the list is empty and a warning is logged.

### Task 3 handoff: Heaven travel + Podium -> Lobby
- Done: ATTAGameMode uses seamless travel. Entering Premiere travels to `HeavenMap`; a later round's Build travels to `BackyardMap` (both in DefaultGame.ini `[/Script/TakeTwoAfterlife.TTAGameMode]`); Podium stays in Heaven. `RequestPhase(Lobby)` from Podium calls `tta::returnToLobby` (scores 0 for players still in, round 0) and travels to `TTASessionSubsystem` LobbyMap with the same session/room code. Match machine, scores and Layout ride in `UTTAMatchCarry` (GameInstance subsystem); cosmetics in new `ATTAPlayerState` (replicated, `CopyProperties`), set from the menu's look carousel. Heaven added to MapsToCook. Core test: podium -> lobby, scores reset, players kept.
- Left (Windows): build Heaven.umap (until it exists travel logs an error and the match stays in the Backyard); PIE travel check. Nothing calls `RequestPhase(Lobby)` after the Podium yet: hook it to the cinema's podium end or a host button. The menu should open on the Lobby screen when it loads in a networked world.
- Surprises: films aren't held by the game mode yet (cinema cuts are passed in by the caller), so nothing to carry for them; when films land, keep them in UTTAMatchCarry. Seamless travel can't be tested in PIE single-process: use standalone/-game with 2 clients.

### Task 4 handoff: Menu restyle
- Done: `UTTAMenuWidget` root is an Overlay (Sky border at 50% opacity under the switcher, so a 3D backdrop can show). Each screen sits in a centred `USizeBox` (`ScreenPanel`): 520 px, or 1080 px for Lobby / How to play. Title screen has its own top row: 96 pt title tilted -4 deg, "Director's Cut" tag under it. Buttons are 480 x 72 `USizeBox`es with 12 px gaps, `FSlateRoundedBoxBrush` (radius 18), hover/pressed = fill lerped toward Ink (25% / 45%). Lobby: players column (room code, waiting) | cosmetics column (swatch + COLOUR/HAT/FACE), START/SETTINGS/LEAVE in a bottom row. Settings rows share `SettingRow` (label 180 px, control 280 px), mic meter included. Fonts load from `/Game/UI/Fonts/LilitaOne` and `/Game/UI/Fonts/Nunito`; if missing, a warning is logged and the engine font is used. No member, handler or delegate names changed; `TTAUIMenuFlowTest.cpp` untouched.
- Left (Windows): UE build, run `TTA.UI.MenuFlow`, and a visual check at 1080p (title tilt, lobby fit, combo widths). Import the two fonts as UFont assets named exactly `LilitaOne` and `Nunito`.
- Surprises: no `Content/` in the repo, so the font assets can't be checked here. Lobby has no player-list widget yet; the players column holds the room code and ready text.

## Task 4: Synth SFX pack — handoff
- Done: `Scripts/make_sfx.py` synthesises 10 SFX (seeded, deterministic) as 16-bit 44.1 kHz mono WAVs in `Audio/SFX/`, 10 ms fades, -1 dBFS peak; WAVs committed. `Scripts/tests/test_make_sfx.py` runs the script into a temp dir and checks format, duration (±10%), peak ≤ 0 dBFS. CREDITS.md row added (in-house, CC0).
- Left: import the WAVs in UE as the `SFX_*` cues (`trap_place.wav` → `SFX_TrapPlace`, etc.) and listen-test; drop the old Freesound SFX row in CREDITS.md if these replace it.
- Surprises: file names follow the task (snake_case), not the `SFX_*` ids in `Core/src/AudioCues.cpp`, so import must rename. The sandbox has no cmake, so `make test` stops before the Python tests; they were run directly.

### Maps task 1 handoff: Shared map library
- Done: `Scripts/maplib/` (grid + corridor read from Placement.h, roles/Prop/footprint/palette, kill plane, credits, validate). `backyard_layout.py` keeps only its zones, kits, dressing, start and gate and calls maplib; public names unchanged, so `build_backyard_map.py` and old tests work as before. `Scripts/tests/data/backyard_golden.json` is the old script's output; `test_maplib.py` checks the new output equals it exactly (props, start, gate, kill plane, zones, kits, meshes), plus role/gap/kill-plane failure cases. `docs/design/path-assumptions.md` lists straight-corridor code.
- Left: Segment + `generate(map, seed)` (task 2). A Core change task is needed before House: `inCorridor` bounds and 45° rotation steps block turned paths.
- Surprises: repo tests run with `unittest` (no pytest in the sandbox); new tests are unittest-style and also run under pytest. `maplib.validate` still checks a straight +X path.

### Maps task 2 handoff: Segments and seeds
- Done: `Scripts/maplib/segment.py`: `Pose` (x, y, z, heading deg), `Segment` (length, exit turn, exit rise, local kill Boxes, local build cells), `place()` (local -> world, optional mirror), `MapDef` (sections of variants), `generate(map, seed)` (seeded `random.Random(f"{name}:{seed}")` picks a variant and a mirror per section, chains exit -> entry), `check_segment` (turn multiple of 15 deg, step up <= 1.5 m, cells on path and on the 0.5 m grid) and `check_layout` (chain <= 1 cm / <= 0.5 deg, length 54-75 m). `Scripts/tests/test_maplib_segment.py` uses a 3-section test map: seeds 0-99 pass, same seed same output, gap and heading-misaligned chain fail, under-1 cm passes, bad segments and short paths fail.
- Left: map scripts (tasks 3-5) build their props per segment and should call `check_layout` plus the per-section prop checks; `maplib.validate` still checks only a straight +X prop list. Build-cell budget (Req 6) is not checked here yet. Core change for turned paths still pending (see task 1).
- Surprises: the turn is a pivot at the exit centreline; segments that turn need a corner floor piece so the outer edge has no gap. Width comes from Placement.h (8 m), not per segment.

### Maps task 3 handoff: House layout
- Done: `Scripts/maps/house.py`: `HOUSE` MapDef, 5 sections in theme order (lawn, shed/drainpipe, bathroom window, counter/sink, bathtub), 2 variants each; `build(seed)` gives start (section 1), finish gate (1 m before section 5 ends), a kill plane spanning the turned path, extra kill volumes and world build cells; `validate()` runs `check_layout` plus Req 5-6. `maplib.grid.budget()` reads kBudget from Placement.h. `Scripts/tests/test_house_map.py` validates seeds 0-99.
- Left: props/dressing per segment and the .umap build; Sky and Pirate (tasks 4-5) can copy this shape. Core change for turned paths still pending.
- Surprises: "within kBudget rules" is read as total build cells per layout <= kBudget (100); the real per-player trap cost check stays in Core. Kill plane is computed from the chained path's bounds, since `maplib.kill.kill_plane` assumes a straight +X path.

### Maps task 4 handoff: Sky Construction layout
- Done: `Scripts/maps/sky.py`: `SKY` MapDef, 5 sections in theme order (crane jib, scaffolding, girders, hook ride, sandpit/dumpster), 2 variants each; `build(seed)`/`validate()` same shape as house.py. Fall gaps are the extra kill volumes; every hook-ride variant has one. `Scripts/tests/test_sky_map.py` validates seeds 0-99.
- Left: props/dressing and the .umap; Pirate (task 5). Core change for turned paths still pending.
- Surprises: none; sky.py duplicates house.py's build/validate. If Pirate copies them too, move them into maplib.

### Maps task 5 handoff: Pirate Dungeon layout
- Done: `Scripts/maps/pirate.py`: `PIRATE` MapDef, 5 sections in theme order (jail cell, torch corridor, sewer, dock planks, ship deck), 2 variants each; `build(seed)`/`validate()` same shape as sky.py. Every sewer variant has water, so every layout has an extra kill volume. `Scripts/tests/test_pirate_map.py` validates seeds 0-99.
- Left: props/dressing and the .umap; maps + lobby docs (task 6). Core change for turned paths still pending.
- Surprises: build/validate are now copied three times (house, sky, pirate); a follow-up should move them into maplib. Not done here to keep this patch to one change.

### Maps task 6 handoff: Maps + lobby docs
- Done: `docs/design/maps.md` (shared library; per map: route, every variant with length/turn/rise/kill volumes/build cells, start/finish, kill zones, build zones, CC0 packs from Kenney, KayKit and Quaternius with links), `docs/design/lobby.md` (podium -> gate -> fall as load screen until all loaded or 20 s -> server seed -> land at Start; late joiners drop in at Start), `Scripts/check_doc_links.py` (browser User-Agent, follows redirects, prints `<status> <url>`, exits 0 only if all 200). `Scripts/tests/test_design_docs.py` checks every map has every field and names every section and variant.
- Left: the lobby flow is design only; no UE code for the load gate or seed replication yet. Core change for turned paths still pending.
- Surprises: no pytest or cmake in the sandbox; the doc test is unittest-style like the others. maps.md is hand-maintained, so adding a variant fails the doc test until its table row is added.

### MVP task 1 handoff: Backyard uses imported kits
- Done: `backyard_layout.py` adds `THIRDPARTY_ROOT`, `MISS_REPORT`, `norm_name`/`resolve`/`closest`/`resolve_all`/`miss_report`. `build_backyard_map.py` lists `/Game/ThirdParty/<kit>` and uses those meshes first; FBX import only for meshes not found there; cube last; every fallback is written to `Saved/backyard_miss_report.txt`. PlayerStart, TTAFinishGate, TTAKillPlane, KillZ unchanged. `Scripts/tests/test_backyard_kits.py` resolves all layout meshes against Content/ThirdParty: 0 misses.
- Left: run `py Scripts/build_backyard_map.py` in the editor on the user's PC and check the report says "no misses" (UE side untested here).
- Surprises: current layout names already match ThirdParty asset names exactly; the `building-type-a` mismatch in the plan is not in this layout, but normalisation covers it. `make test` stops at cmake in the sandbox, so the Python suite was run directly.

### MVP task 2 handoff: No avatar or controls after Host
- Done: cause was `ATTAGameMode` leaving `DefaultPawnClass` at the engine's ADefaultPawn (no runner bindings, sphere), plus the Title's UIOnly input mode surviving travel. Default pawn is now `RunnerPawnClass`; `RestartPlayer` override sets start location + human form; Lobby without travel restarts runners; `ATTAPlayerController::BeginPlay` sets GameOnly and hides the cursor. Config already correct (RunnerMesh=SK_Angel, 3 maps, WASD/Space); `test_runner_spawn.py` locks it. DEVLOG has the 5-step PC check.
- Left: run that check on the user's PC (UE compile untested here).
- Surprises: none in config; the plan was right that input ini wasn't the cause.

### MVP task 3 handoff: Bot brains
- Done: `Core/include/tta/BotBrain.h` + `src/BotBrain.cpp`: `BotBrain(playerId, seed)` with `build(Layout&)` (random traps via `Layout::placeTrap`, so budget/bounds rules stay in Placement; skips traps it can't afford; 3 cameras), `planRun()` (`RunnerSkill` Clumsy/Average/Pro + 5-30% deliberate-fail chance), `rate(RatingBook&, cuts)` (skips cuts it edited or built). `tests/BotBrainTests.cpp`: budget/cap over 200 seeds, determinism, no self-rating, run-plan ranges.
- Left: UE side: bot controllers that drive the runner pawn from `RunnerPlan`, and Play Solo wiring.
- Surprises: no skill type existed in Core, so `RunnerSkill` is new. RNG is SplitMix64, not `<random>` distributions, so a seed matches across MSVC and GCC. Sandbox has no cmake; `pip install cmake` makes `make test` run fully.

### MVP task 4 handoff: Bot pawns + Play Solo
- Done: `ATTABotController` (Public/Private) + Core `tta::BotRoute` (steering, doctest `BotRouteTests.cpp`), `BotBrain::vote`/`rollFail`. Game mode: `?bots=N` (max 3), `AddBots`, bots build in `EnterBuild`, run in `EnterRun` (now iterates all controllers), get `autoEdit` cuts in `EndRun`, keep ids 1000+ across travel. Menu PLAY SOLO → `HostSolo(3)`. Backyard markers tagged `TTARoute`. Automation `TTA.Bots.SoloRound`; `Scripts/tests/test_bots.py`.
- Left: compile + run `TTA.Bots.SoloRound` and the DEVLOG solo check on the user's PC; rebuild Backyard so markers get the tag. Premiere is still not wired into the Heaven map (nothing calls `ATTACinema::StartPremiere` in game), so bots' `RateCurrent` is ready but only the test calls it; that wiring is a later task.
- Surprises: task 3 never added `BotBrain.cpp` to the UE wrapper folders (`Source/TTACore/Private`, `Private/CoreLib`), so it wouldn't have linked in UE; added to both, and `test_bots.py` now checks every Core source has a game-module wrapper (`CoreMenuFlow` was never in TTACore, so that folder isn't checked). `RunnerSkill` only shows through fail chance (no speed change). Bot cuts use camera 0 as the Heaven Cam (editor's no-camera fallback); map to real cameras when the editor exposes indices server-side.

### MVP task 5 handoff: Automation crashes
- Done: MenuFlow crash cause = `NativeOnInitialized` -> `RefreshVideo()` -> `check(UGameUserSettings)` + RHI resolution query in a bare test world. Moved to Settings open, `check`s -> logged skips, test local player outered to the game instance. Premiere advance and respawn timing re-read against Core `MatchStateMachine`/`PawnLife`: consistent, no change. DEVLOG lists it with `RunTests TTA.`.
- Left: run `RunTests TTA.` on the user's PC; the crash log (`Saved/Automation/TTA3.log`) is not in the repo, so the cause is from code reading, not a stack trace. Wire the cinema to call `EndPremiere` in game.
- Surprises: `make test` needs `pip install cmake` in the sandbox; with it, all pass.

### MVP task 6 handoff: Cosmetics table
- Done: `FTTACosmeticRow` (`Public/TTACosmetics.h`: HatMesh, SocketOffset, Rotation, Scale, FaceTexture, PaletteIndex). `ATTARunnerPawn::ApplyLook(hat, face)` reads `CosmeticsTable` (DefaultGame.ini -> `/Game/Data/DT_Cosmetics`): row Hat sets hat mesh/transform on socket `HatSocket` ("head") + `Tint` from kPaletteHex; row Face sets `FaceTexture` on the runner MIDs. Called in BeginPlay and OnPlayerStateChanged. `Scripts/cosmetics_rows.py` (engine-free rows/CSV, names parsed from MenuFlow.h kHats/kFaces) + `Scripts/build_cosmetics_table.py` (imports missing SM_Hat_*/T_Face_* from models/, fills the table). `test_cosmetics_rows.py`.
- Left: on the user's PC: compile, run the script, check SK_Angel has a `head` socket and the toon material has `FaceTexture`/`Tint` params (missing params are no-ops), tune offsets per row. Look changes in the lobby menu after spawn don't re-apply (no OnRep on Hat/Face).
- Surprises: no hat/face uassets existed in Content; the script imports them. Faces are chosen independently of hats, so a row carries one of each and the runner reads two rows.

### MVP task 7 handoff: Look pass + release checks
- Done: `Scripts/apply_world_look.py` (editor): one unbound PostProcessVolume tagged `TTAWorldLook` + preset, `M_TTA_PP_Outline` blendable, preset MPC values, per map in `toon_spec.LOOK_MAPS` (Title/Backyard = Course, Heaven = Heaven); reruns replace the volume. `Scripts/release_checks.py`: Title must be GameDefaultMap, Title + every `TTAGameMode` `*Map=` must be in MapsToCook; run first in `package_win64.bat` and again in `zip_package.py` main. Size check already existed (>= 1 GiB fails, zip removed); boundary test added. SUBMISSION.md has a 10-step launch checklist. `test_release_checks.py`.
- Left: on the user's PC: run `swap_to_toon.py /Game/ThirdParty` and `apply_world_look.py` (UE side untested here), then the checklist.
- Surprises: MPC values are global, not per map: the script only previews them in the editor; in game only `TTACinema` calls `ApplyWorldLook` (Heaven), so Course values come from the MPC defaults that `create_toon_assets.py` writes. Swapping /Game/ThirdParty can't run in the sandbox, so it is checklist step 2.

### MVP task 9 handoff: Premiere never ends
- Done: `ATTAGameMode::BeginPremiere/OnCinemaStep/FinishPremiere`: server starts the Heaven cinema with bot cuts after travel, bots rate, Podium/Done -> `EndPremiere(ratingPoints)`, timeout = premiere length + 3 s. `TTAPlacementMarker` ItemMeshes -> `/Game/ThirdParty/KayKitPrototypeBits/*`. `Scripts/tests/test_premiere_end.py`. DEVLOG solo-round-to-podium check.
- Left: UE compile + the DEVLOG check on the user's PC. Human players' cuts aren't sent to the server yet, so only bot cuts premiere, and humans have no rating UI/RPC. Bot cut builder = the bot itself (trap owner per cut not tracked).
- Surprises: nothing called `StartPremiere` either, so the fix starts the cinema too, not just listens. The kit folder is `KayKitPrototypeBits` (no spaces). Timeout counts intro/rating/reveal too; cut length + 3 s alone would cut the cinema off mid-rating.

### Task 1 handoff: Backyard + Heaven layout pass
- Done: `backyard_layout.py` gets 10 more route-side dressing pieces (`_route_dressing`) and two flat shortcut branches (`SHORTCUTS`, new maplib role `shortcut`, Coin_A stones bowing to |y| = 2.5 between markers 16-24 and 40-48, no TTARoute tag). Only meshes already in the layout, so the resolver test stays at zero misses and CREDITS needs nothing. New `Scripts/heaven_layout.json` (podium stanchions + side clouds, 8-cloud spectator ring around the podium) read by `dress_heaven.py`. Tests: `test_backyard_dressing.py`, `test_heaven_layout.py`.
- Left: run `build_backyard_map.py` and `dress_heaven.py` in the editor; Heaven footprints in the JSON are estimates, check against the real mesh bounds.
- Surprises: there was no Heaven layout JSON (positions were hard-coded), so this adds one. `backyard_golden.json` props/meshes were regenerated since the layout grew on purpose; start, gate, kill plane, zones, kits are still asserted unchanged. No NavMesh bounds exist in the scripts, so nothing to keep there.

### Task 2 handoff: UI style wiring
- Done: `UTTAUIStyle` (`Public/TTAUIStyle.h`, Config=Game, `[/Script/TakeTwoAfterlife.TTAUIStyle]` in DefaultGame.ini): palette-only `Colour(index)`, Lilita One / Nunito via `DisplayFont`/`BodyFont`, `SafeArea` 96x54 at 1920x1080, `MenuBackground`/`HUDBackground`/`ResultsBackground`/`PodiumBackground` (empty = palette fill, set a Texture2D path to use an image), shared `AddCard`/`AddLabel`/`MakeFont`. `UTTAMenuWidget` now goes through it (palette, fonts, card, background, safe-area padding). `ATTABuildHUD` restyled: top bar on every phase with phase + round (left), timer (centre, Red at 10 s), budget (right, while building), build panel below, all scaled from 1080p inside the safe area. New `UTTAResultsWidget` + `UTTAPodiumWidget`, added by the HUD in Podium. `docs/art/palette.md` style sheet. `Scripts/tests/test_ui_style.py` (every widget uses the style + safe area, no colour outside the palette, doc/ini/fonts match).
- Left: compile on the user's PC (not compiled here). Canvas text scale assumes `UFont::GetMaxCharHeight()` is the font's pixel height; check HUD text size at 1080p. Results points use `APlayerState::GetScore()`, which nothing in the match sets yet; feed it from `MatchScore` totals. Podium names show "Player N" (ids from `ATTACinema::GetPodium()`).
- Surprises: `docs/art/palette.md` (the "task 8 style sheet") did not exist on the branch, so it was written from `toon_spec.PALETTE` + the task 24 font choice. No results or podium widget existed; both are new. The old HUD used engine Red/White/Green/Yellow; now palette Red/Bone/Grass/Yellow.

### Task 5 handoff: Editor won't compile on UE 5.5
- Done: the four reported errors fixed at the cause (missing IWYU includes; `if (...) UE_LOG(...); else` in `ATTAGameMode::EnterRun` braced, since UE_LOG is a braced macro; MenuFlow test `;` was inside a `//` comment). IWYU pass over every Source file; Build.cs public deps (AIModule, NavigationSystem, GameplayTasks, UMG, Slate, SlateCore, EnhancedInput; UMG/Slate/SlateCore moved from private). `.uproject` was already "5.5"; README/DEVLOG/SUBMISSION/OVERNIGHT/scripts already named only 5.5 / `UE_5.5` (no CLAUDE.md/AGENTS.md exist). DEVLOG has the PowerShell Build.bat command. `Scripts/tests/test_ue55_compile.py` checks all of it.
- Left: run that Build.bat on the user's PC (not compiled here). The include check covers a fixed list of engine types; an error on a type outside it means adding it to `HEADERS` in the test.
- Surprises: the reported line numbers (297 etc.) don't match the branch; the only orphaned-else pattern in TTAGameMode.cpp is the bots' StartRun (~line 377). EnhancedInput is now a dependency but no code uses it yet; the plugin is on by default in 5.5.

### Task 6 handoff: TTAUIStyle.h C2487
- Done: the header never had per-member `TAKETWOAFTERLIFE_API` and had no .cpp definitions; the errors were the comma-listed `static constexpr` declarators (every name after the first). Split to one per line, values unchanged. New `Scripts/tests/test_exported_members.py` (no member API macro, no comma-listed static members in any exported class/struct in Source/). DEVLOG has the Build.bat command.
- Left: run Build.bat on the user's PC (MSVC not available here).
- Surprises: the other non-class `TAKETWOAFTERLIFE_API` uses are free functions in namespaces (TTAReels.h, TTAPlacementMarker.h), which is valid; no other header has the pattern.

### Task 7 handoff: apply_world_look.py on UE 5.5
- Done: dropped `KismetMaterialLibrary`/`EditorLevelLibrary`; Course preset now written into `MPC_TTA_World` defaults and saved (`write_mpc_defaults`); editor world via `UnrealEditorSubsystem`; outline volume still placed in all three `LOOK_MAPS`. `Scripts/tests/test_world_look_ue55.py`. DEVLOG has the run / restart / check steps.
- Left: run it in the UE 5.5 editor on the user's PC (no Unreal here).
- Surprises: no other Scripts/ file used Kismet*/EditorLevelLibrary. One MPC asset can only hold one default set, so Heaven's values can't be baked into it; Heaven relies on the runtime `ApplyWorldLook` (C++ `UKismetMaterialLibrary` there is fine, it's only missing from Python).

### Plan 3 task 1 handoff: Layout places actors wrong
- Done: two causes. Backyard: `build_backyard_map.fit()` called `set_relative_location` on `static_mesh_component`, which is the StaticMeshActor's root, so the bounds offset overwrote the spawn location and every actor landed near (0,0,0). Heaven: `dress_heaven.sensible_scale()` treated any mesh under 20 cm tall as a metres import and scaled it 100x; the 2 cm red carpet got 100,100,100 and its x was then computed from that 100x length (= -11945). New shared `Scripts/layout_spawn.py`: `fit_scale` (bounds fit to a target size in cm, raises outside 0.05-10), `placement` (actor location so the mesh bottom-centre lands on the spot, yaw-aware), `spawn_fitted` (spawn_actor_from_object at that location, actor scale, TTA_Layout tag), `delete_previous` (destroys TTA_Layout actors, never PlayerStart/TTAFinishGate/TTAKillPlane/NavMeshBoundsVolume/RecastNavMesh/WorldSettings). Both scripts delete old TTA_Layout actors first. Backyard no longer destroys PlayerStart/gate/kill plane; it only spawns them (and sets KillZ) when missing. Heaven sizes come from `HEAVEN_FIT` + the JSON (carpet = the protected RedCarpet box at x 390, 480x150 cm). Floors split into <=8 m tiles and marker stones 3->5 cm so a 1 m placeholder cube stays inside 0.05-10; `backyard_golden.json` regenerated for that. `Scripts/tests/test_layout_spawn.py`.
- Left: run both scripts in UE 5.5 on the user's PC (no Unreal here). First rerun after this patch: actors from older runs carry no TTA_Layout tag, so delete the old Backyard StaticMeshActors and the old Heaven podium/clouds/carpet/TargetPoints by hand once. Target sizes for trophy (80 cm), halo (60 cm), podium (300x300 cm), stanchions (100 cm), sky clouds (6 m) are my picks; tune in `layout_spawn.HEAVEN_FIT`.
- Surprises: Heaven podium actors used to be reused by label; now deleted and respawned each run. Meshes spawn with `spawn_actor_from_object`.

### Plan 3 task 2 handoff: Bots don't run the round
- Done: mic cause fixed (bots no longer record; id cached at capture start, so the lobby pawn destroyed in EnterRun no longer logs "no PlayerState"). Verified the requested pieces already exist: `bWantsPlayerState = true`, NavMesh-free straight-line steering via `tta::BotRoute` + `AddMovementInput`, Run ends into Edit on AllFinished or timer+SmiteHoldSeconds with `StopRun` + `FillCut` for every bot. Locked by `Scripts/tests/test_bots_round.py`. `LogTTABots` logging added.
- Left: run `Automation RunTests TTA.Bots` in UE 5.5 on the user's PC (no Unreal here). The :90/:98/:101 failures could not be reproduced in the sandbox; if they remain, the LogTTABots Verbose lines show whether the bot ticks, its target and its direction.
- Surprises: the task description assumed the PlayerState flag, NavMesh fallback and Run->Edit were missing; all three were already on the branch. The only reproducible defect in source was the mic one.

### Plan 3 task 3 handoff: Menu test, resolution and materials
- Done: (a) MenuFlow test used a bare CreateWorld + an uninitialized `NewObject<UGameInstance>(World)`; it now uses `NewObject<UGameInstance>(GEngine)->InitializeStandalone()` and takes that world, so the GI-outered LocalPlayer is legit. (b) `tta::clampResolutionIndex` (core, doctest); `ApplyResolution` clamps instead of warning; the "17 of 17" warning came from the automation test probing index == count. (c) `BeginPlay` MIDs only when the skeletal mesh is loaded and the slot has a material; hat MID guarded. DefaultGame.ini already pointed at SK_Angel. (d) `Scripts/fix_prototype_material.py` sets prototype_texture to Masked (Material, or MIC override). Tests: `Scripts/tests/test_menu_materials.py`.
- Left: run `Automation RunTests TTA.` and the editor script in UE 5.5 on the user's PC; confirm SK_Angel shows 2 material slots (palette + face, per models/build_angel.py).
- Surprises: the material loop already used GetNumMaterials(); the invalid-index log means slots existed with no mesh/material behind them (mesh not loaded), so the guard is on the mesh asset + GetMaterial.

### Plan 4 task 1 handoff: Heaven lobby + gate start
- Done: `LobbyMap` = Heaven. `tta::HeavenGate` (open / ready count / all-ready or 20 s timeout, `Core/tests/HeavenGateTests.cpp`). `ATTAGameMode::OpenGate` (host START via `ATTAPlayerController` Enter / Gamepad_Special_Right; Solo at StartPlay, bots ready at once), `MarkReady` (from `ATTAFinishGate` in Lobby), Lobby Tick -> `RequestPhase(Build)`. Build now travels to Backyard unless already on it (`IsOnMap`). GameState replicates gate open + n/max; HUD card. `dress_heaven.py` spawns the gate trigger on SM_Heaven_RedCarpet (the layout has no pearly gate piece). `Scripts/tests/test_heaven_lobby.py`.
- Left: run `py Scripts/dress_heaven.py` and the DEVLOG manual check in UE 5.5 (no Unreal here, so UE code is uncompiled). Gate arch has no "open" animation yet; only the HUD shows it is open. Trigger box (100x800x400 cm) may need resizing to the carpet.
- Surprises: Heaven had no gate trigger; `SM_Heaven_PearlyGate` is only in the dress colour table, not in heaven_layout.json, so the trigger sits on the red carpet. Round 1 used to assume it already stood in the Backyard; `IsOnMap` keeps that for automation tests that build in a Backyard world.


### Plan 5 task 2 handoff: One map, 3 rounds, traps stay
- Done: `tta::Layout::beginBuild(round)` + `Placement::round`; `spent`/`remaining` count only the current round's traps, so placements stay and every player gets a fresh 100 each Build. Layout JSON writes `round` per placement (missing = 0 for older files) and restores the current round. `ATTAGameMode::RequestPhase(Build)` calls `beginBuild` before travelling to the Backyard so the carry keeps it. Doctests in `Core/tests/PlacementTests.cpp`: round 2 starts with round 1's placements, budget resets each round, match goes to Podium after round 3.
- Left: UE side is uncompiled here; run the editor build and `Automation RunTests TTA.` on the user's PC, and play 3 rounds to confirm traps/cameras reappear after the Heaven<->Backyard travel.
- Surprises: most of the task was already on the branch. `Rounds=3` was already in DefaultGame.ini, every Build already travels to the Backyard, the Layout already rode `UTTAMatchCarry` across travel, and `EnterRun` already respawns runners at `FindPlayerStart` and re-inits every trap actor (`InitFromPlacement` -> pose 0 = re-armed/reset) each Run. The only real gap was the budget counting every round's traps.

### Plan 5 task 3 handoff: Score board, podium, back to lobby
- Done: Core `tta::rankOrder` (Scoring; `MatchScore::podium` now uses it), `tta::ScoreBoard` (rows in new-rank order with old/new rank; eased bar value + slot), `tta::PodiumShow` (Travel -> Statues -> Hold -> Lobby; `statuesStand` = Podium/Lobby). Doctests `ScoreBoardTests.cpp`, `PodiumShowTests.cpp`. Cinema keeps running totals (`SetBaseTotals` + rating points) and builds the board on each Reveal step (the existing 4 s `revealSeconds`); HUD draws it. GameMode: Podium -> `StartPodium` -> `SpawnStatues` (RunnerPawnClass + `FreezeAsStatue`, at TargetPoints 1st/2nd/3rd) -> `PodiumHoldSeconds` -> `RequestPhase(Lobby)`. `ReturnToLobby` no longer travels when already on the lobby map (Heaven), so statues stay; `ClearStatues` on the next Build. `Scripts/tests/test_podium_flow.py`.
- Left: UE side uncompiled here; run the editor build, `Automation RunTests TTA.` and the DEVLOG solo check. The board reads the cinema's premiere, which (like the podium list) lives on the server: on a listen-server client it may show nothing until the cinema state replicates. Body tint uses the hat's `TintParam` on the body MIDs; if SK_Angel's palette material has no such parameter the statue keeps its default colours. Statue pose = whatever frame the idle anim is on when paused.
- Surprises: the Reveal step already lasted 4 s, so the board replaced it instead of adding a step. Premiere already runs in Heaven, so the podium's Travel step only fires if the premiere fell back to another map. `ReturnToLobby` used to reload Heaven, which would have wiped the statues.

### Plan task 4 handoff: Proximity voice chat
- Done: voice ini keys were already set on the branch; added config radii `VoiceFullVolumeCm=500` / `VoiceSilentCm=2500` (sphere inner radius + falloff) to the runner's runtime `USoundAttenuation`, [M] `ToggleVoiceMute` -> `PlayerController::ToggleSpeaking`, `Scripts/tests/test_proximity_voice.py` (ini keys, radii, unique mute key, mute never touches MicRecorder), DEVLOG two-window test.
- Left: UE side uncompiled here; run the editor build and the DEVLOG two-window test. Attenuation is a runtime object, not a .uasset, so tuning lives in DefaultGame.ini. Mute state is not shown on the HUD yet.
- Surprises: the VOIPTalker + attenuation were already in `OnPlayerStateChanged`, but only set FalloffDistance=2500 (so it faded from 0 m, not 5 m). VOIP and TTAMicRecorder open separate `IVoiceCapture`s on the default device; Windows allows that, but if a device refuses shared capture the replay mic will be silent and needs a look.

### Plan 6 task 5 handoff: Menu rework, structure only
- Done: Title screen is now PLAY SOLO / HOST GAME / JOIN (CODE OR IP) / SETTINGS / QUIT (Reels and How to play buttons and their click handlers removed from the title). New `UTTAUIStyle` art slots `MenuLogo`, `ButtonNormal`, `ButtonHovered`, `ButtonPressed` (empty keys in DefaultGame.ini, next to the existing `MenuBackground`) plus `LoadSlotTexture`; the menu shows the logo image instead of the text title and uses button textures instead of the palette rounded boxes when they are set. `Scripts/tests/test_menu_structure.py` checks button order, a declared handler per title button, and that every slot exists in the style header and ini.
- Left: UE side uncompiled here; run the editor build, `Automation RunTests TTA.` and look at the title menu. Drop generated art in by setting the ini paths. Button textures draw as Image brushes, so 9-slice margins may be needed once the real art exists.
- Surprises: `MenuBackground` already existed from the UI style task. The Reels and How to play screens still exist and are reachable through `Open()` (automation test uses them), but nothing on the title links to them now.

### Build watch handoff (Nov 15): main compiles, TTA automation 16/16
- Done: built main in UE 5.5 (no compile fixes needed) and ran `Automation RunTests TTA.` for real. Two failures, both test-harness bugs: SoloRound ticked the world many times in one `GFrameCounter` (now increments it per tick); MenuFlow outered `ULocalPlayer` to the game instance (now `GEngine`, as `Within=Engine` requires). `Scripts/tests/test_menu_init.py` flipped to match the engine.
- Left: nothing failing. Remaining automation warnings are benign (`CleanupWorld ... missing call to EndPlay` from test worlds, `SetStaticMesh ... Mobility is Static` on the test floor, empty resolution list under `-nullrhi`).
- Surprises: any automation test that calls `World->Tick` in a loop must `++GFrameCounter` each iteration, or actors tick only once. The earlier "bots don't move" reports from this test were this, not the bot code.

### Merge handoff (Nov 15): main merged into plan branch, compiles, TTA automation 16/16
- Done: merged `origin/main`; DEVLOG.md / plan.md conflicts resolved by keeping both sides. Compile fixes for the branch's feature code: `#include "UObject/Package.h"` in `TTAGameMode.cpp`, `InputCore` added to `TakeTwoAfterlife.Build.cs`. No feature code removed. Editor build, `Automation RunTests TTA.` (16/16) and `Scripts/test.ps1` all pass.
- Left: the per-task manual checks in DEVLOG (Heaven gate, 3 rounds with persistent traps, score board/podium statues, two-window proximity voice, title menu) need a played session; `py Scripts/dress_heaven.py` has not been run.
- Surprises: no automation test covers the new lobby gate, podium or voice paths, so 16/16 only shows the older flows still work with them in place.

### Plan task 7 handoff: Backyard fit on kit units and axes
- Done: the 600x800x20 box is the patio `Primitive_Floor` tile (6 x 8 x 0.2 m, `backyard_layout._floor`); KayKit imports it as [4, 1, 4] (metres read as cm, Y-up), so the raw per-axis fit gave (150, 800, 5). New `layout_spawn.fit_mesh`: x100 when the largest bound is under 10 cm, then for box fits match the clearly-thinnest bound to the thinnest target axis (roll 90 about X for Y-up, pitch 90 about Y for X-up), then the same guarded `fit_scale`. `spawn_fitted` spawns with that rotation and bottom-centres from the rotated box; `spawn_or_skip` logs `skipped <label>: ...` and returns (None, None) on `BadFit`. Backyard `place()`, the gate arch fit and `dress_heaven.spawn_mesh` use it. Tests: `Scripts/tests/test_fit_units_axes.py` + `tests/data/mesh_bounds.json`.
- Left: `mesh_bounds.json` is the logged floor value plus nominal kit sizes, not an editor dump; take a real `get_bounding_box()` listing and replace it. Run `build_backyard_map.py` and `dress_heaven.py` in the editor and check the patio slab is flat and the roll sign keeps it bottom-on-ground (UE roll handedness is not checkable here). A skipped Podium leaves Trophy/Halo at podium height 0.
- Surprises: height/width fits use Z regardless of up axis, so a Y-up kit prop fitted by height is sized off its depth; only box fits get the axis match, per the brief.

### Plan task 9 handoff: Lobby spawn + camera (code only)
- Done: `tta::pickSpawn` (Core/SpawnPlacement, doctest): 300 cm above the floor hit, then +100 steps up to +400 more (700 total), then the next PlayerStart. `ATTAGameMode::FindSafeSpawn` traces WorldStatic down from the chosen start then all others, tests the runner capsule on ECC_Pawn, and feeds `RestartPlayer` (via `RestartPlayerAtTransform`) and `EnterRun`; all blocked -> old behaviour. `ATTARunnerPawn` now has `CameraBoom` (350 cm, `bDoCollisionTest`, probe 12 cm on ECC_Camera) + `FollowCamera`; filming swaps to Handheld. `dress_heaven.py` exits with a warning when `TTA_HandDressed` is on World Settings or any actor (`Scripts/hand_dressed.py`). Tests: `Core/tests/SpawnPlacementTests.cpp`, `Scripts/tests/test_lobby_spawn_camera.py`.
- Left: editor compile + DEVLOG "Lobby spawn + camera check"; put the `TTA_HandDressed` tag on Heaven's World Settings in the editor (not done here: no .umap edits). dress script not run.
- Surprises: the runner had no spring arm at all, only the inactive Handheld camera, so the view fell back to the pawn's eyes. "Offsets up to 400 cm" read as extra lift on top of the 300 cm.

### UI refactor task 1 handoff: generated UI art + shared style system
- Done: 18 generated alpha PNGs in `Art/UI/` (`T_UI_*`; magenta chroma key, despill, crop, max 512 px wide, backdrop 1536x1024 opaque RGBA). `Scripts/import_ui_assets.py` imports them to `/Game/UI/Generated/` (TC_EditorIcon, no mips, UI group, never stream). `TTAUIStyle` extended in place: `ETTAColor/ETTAFont/ETTAButtonKind/ETTAPanelKind/ETTAAsset`, `Color`, `Font`, `Space`, `PadAfter`, `Brush` (9-slice from the texture, flat palette fallback + warning) and `MakeButton/MakePanel/MakeLabel/MakeIconCount/MakeBanner/MakeSlider/MakeTextField`. `docs/design/ui-style.md` lists every token, size, padding, slice and asset. `UTTAEditorWidget` uses the builders. `Scripts/tests/test_ui_style_guide.py` checks doc <-> code <-> `Art/UI` both ways, PNG alpha, and bans `FLinearColor(` / `FSlateFontInfo(` / numeric `FMargin(` / texture paths in widget .cpp files.
- Left: editor build + import run on the user PC (DEVLOG steps). 9-slice margins are estimates from the art; tune in-editor. Screen tasks 2-6 still to move onto the builders.
- Surprises: `/workspace/ui/assets/` was empty, so this task generated the art itself. The R-S2 ban hit files outside the edit screen too: `TTATimelineWidget` literal colours now use palette tokens (small colour shift: strip Ink, clips Sky/Yellow, playhead Pink, markers Orange/Red/Yellow), and the numeric trailing-pad `FMargin`s in `TTAMenuWidget`/`TTAResultsWidget` use `PadAfter` (same values). Builders take the `UWidgetTree*` first, like the existing `AddCard`/`AddLabel`.
### Plan task 1 handoff: Toon outline alignment
- Done: outline lookups in buffer UV with a buffer-pixel step; `M_TTA_Toon` Default Lit (Emissive = toon, BaseColor = black) so the normal edge test has data. Tests: `Scripts/tests/test_toon_outline.py`.
- Left: run `Scripts/setup_content.ps1` to rebuild the materials, then check TTA_ArtTest in the editor and in PIE at a screen percentage under 100 that ink sits on edges. Default Lit adds scene lighting/specular on top of Emissive; if surfaces look shiny, set Specular 0 / Roughness 1.
- Surprises: none in code; editor result unverified here.

### Plan task 1 handoff: Heaven props sit too low
- Done: placement already bottom-centres from bounds (not pivot); the floor was the problem. `heaven_pieces` gave world Z 0 while the cloud floor is ATTACinema's root, so its top is wherever the cinema actor stands; and `dress_heaven.py` called a missing `S.mesh_bounds`, so it aborted before re-dressing and the map kept older props. Now: `layout_spawn.mesh_bounds`, `fitted_location` (pure math of `spawn_fitted`), `FLOOR_TOP` (cloud disc top = cinema origin, models/build_heaven.py) and `SLAB_LIFT` 0.5 cm for the podium; every piece has `grounded` (sky clouds + Halo float). `dress_heaven.cinema_point` maps layout spots, gate and podium TargetPoints through the cinema transform. Test: `Scripts/tests/test_heaven_grounding.py` on recorded lo/hi (`mesh_bounds.json` `HeavenBounds`, from the .glb files).
- Left: rerun `dress_heaven.py` in the editor and check podium/trophy/stanchions/clouds sit on the cloud and the podium base no longer flickers. If it still flickers at distance, raise `SLAB_LIFT` to 1-2 cm.
- Surprises: no rocks or gems exist in heaven_layout.json, layout_spawn.py or Heaven.umap's names; if they are in the map they were hand-placed and this script doesn't move them. `HeavenBounds` comes from the .glb exports, not an editor dump. 5 `test_ui_style_guide` failures happen without this change too.


### UI refactor task 2 handoff: Title menu + Play page
- Done: `UTTAMenuWidget` builds every screen from `TTAUIStyle` builders (`MakeButton/MakePanel/MakeLabel/MakeSlider/MakeTextField`); the old palette rounded-box buttons, raw sliders and int font sizes are gone. Title = title, "Director's Cut", PLAY / SETTINGS / REELS / QUIT, angel placeholder on the right. PLAY opens the Play page on the existing Join switcher slot (same `Open()`/`Back()` stack as Settings): HOST, PLAY SOLO, one code-or-IP field + JOIN, BACK. JOIN/Enter: text with `.` or `:` fires `OnJoinDirect`, anything else goes through `SubmitJoinCode` (red panel + error on a bad code). Settings gets a SETTINGS heading over the white panel of rows. New builder `MakeAngel` (texture `/Game/Characters/Angel/Face`, panel-brush fallback) + ui-style.md row. `test_menu_structure.py` rewritten for the new title/Play page and a no-raw-widgets check.
- Left: editor compile + click-through on the user PC: each title button, HOST, PLAY SOLO, JOIN with a code and with an IP, BACK from Play and Settings. Confirm `Face` is a Texture2D (else pick `Body`/`Outfit` in `kAngelTexture`). No DEVLOG entry (task limited the diff).
- Surprises: SOLO kept: it calls `HostSolo` (bots), which HOST doesn't. `test_menu_structure.py` hard-coded the old 5 buttons, so it had to change. `MenuScreen` has no Play value; reused `Join` rather than touch Core. Button art slots (`ButtonNormal/Hovered/Pressed`) are no longer read by the menu; generated button art replaces them.

### UI refactor task 3 handoff: Lobby panel reskin
- Done: the Heaven lobby overlay in `UTTAMenuWidget` is one compact white panel (`kLobbyW` 420) on the left edge, built only from `TTAUIStyle` builders: ROOM CODE label + gold `CodePill` panel holding `RoomCodeText`, look swatch, COLOUR / HAT / FACE pickers, START (primary), SETTINGS, LEAVE, then `MakeIconCount(IconReady)` whose label is `WaitingText`. Same handlers and members, so `SetRoomCode`/`SetPlayers`/`RefreshLook` are unchanged. `test_menu_structure.py` gains a lobby test (button order, left alignment, pill, tick, bound members, handlers).
- Left: editor compile; host, walk to the gate and check the panel sits left, the code fits the pill (H2) and the ready line wraps. No style files changed; no DEVLOG entry (task limited the diff).
- Surprises: the old lobby was a two-column wide card with no tick icon; `MakeIconCount` doesn't return its label, so the lobby takes it with `GetChildAt(1)`.

### UI refactor task 4 handoff: Build + Run HUD reskin
- Done: new `UTTABuildHUDWidget` (declared in TTABuildHUD.h) builds the Build/Run overlay only from `TTAUIStyle` builders: phase + round `MakePanel` (left), timer `MakeBanner` with the kTimer face, red at 10 s or less (centre), BUDGET `MakePanel` (right, Grass/Red); lobby ready count as `MakeIconCount(IconReady)`; Run film hint `MakeIconCount(IconMicOn, "[RMB] hold to film")`; build panel with CAMERAS n/cap + four `MakeIconCount` camera-type icons (fixed/pan/follow/trap), item/keys line, ghost result, rejected line, selected trap + 2 settings. `ATTABuildHUD::DrawHUD` creates it once and fills it every frame; hidden in Podium. Same texts, conditions and data as the old canvas HUD; nothing added.
- Left: compile + one Build and one Run on the user's machine. Premiere score board is still canvas-drawn (premiere task's scope).
- Surprises: the HUD was Canvas-drawn, so "builders only" meant moving it to a UMG widget. `make test` needs cmake; the sandbox has none, so only the Python tests (`Scripts/tests`) ran here. The mic icon is static (the HUD never read mic state). Not compiled here.

### UI refactor task 5 handoff: Premiere card reskin
- Done: the premiere overlay moved off the canvas into `UTTABuildHUDWidget`, built only from `TTAUIStyle` builders: a centred NOW SHOWING `MakePanel` card with `ATTACinema::GetIntroLine()` (H2) and "CUT x OF y" (from `tta::Premiere::index()/order()`), and under it a `MakePanel` score board whose rows are P# label + `AddCard` bar (player colour, width from `tta::scoreBarAt`) + value. `ATTABuildHUD::DrawScoreBoard` became `UpdatePremiere`; the canvas helpers (`DrawCard`, `DrawStyledText`, `MeasureText`, `UiScale`, canvas fonts) are removed since nothing else used them. Timing unchanged: card shows while the premiere has a current cut, bars only while `GetScoreBoard()` is non-empty (Reveal), same alpha.
- Left: editor compile + one premiere on the user's machine: card sits under the timer banner, intro line fits, bars grow during Reveal and rows reorder by slot. No style files changed; no DEVLOG entry (task limited the diff).
- Surprises: the mockup PNG could not be viewed in the sandbox, so the layout follows the task text (card + cut x of y + board). `make test` stops at cmake in the sandbox, so the Python suite was also run directly. `test_podium_flow.py` names the HUD function, so it now points at `UpdatePremiere`.

### UI refactor task 6 handoff: Podium + results reskin
- Done: `TTAPodiumWidget` and `TTAResultsWidget` rebuilt from `TTAUIStyle` builders only. Podium = `MakeBanner("AND THE WINNER IS...")` at the top of the safe area over 2nd/1st/3rd `MakePanel` blocks (rank H1, winner in Yellow, "Player N" body). Results = a centred `MakePanel` with a RESULTS H1 and one row per player (rank in the player's colour, name, points), best first. Legacy `AddLabel`/`AddCard` and raw `k*` sizes removed from both; text is `NSLOCTEXT`. Background fill is now Sky (pastel mockups). No buttons; `SetPodium`/`SetRows` and the game mode's auto-return to the lobby untouched. New static test `Scripts/tests/test_podium_results_reskin.py`.
- Left: editor compile + one full match on the user's machine: podium shows, banner/blocks/results panel don't clash (results and podium are separate viewport widgets added by `ATTABuildHUD::ShowResults`), lobby return still fires. No style files changed; no DEVLOG entry (task limited the diff).
- Surprises: podium.png could not be viewed in the sandbox, so layout follows the task text. `make test` stops at cmake in the sandbox, so the Python suite was also run directly.
