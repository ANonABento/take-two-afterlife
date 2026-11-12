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
