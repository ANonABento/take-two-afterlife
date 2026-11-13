# Take Two: Afterlife — Public Dev Log

A party game made in three days for the Epic AI MegaJam 2026, almost entirely by AI coding agents, with one human (Kevin) steering, building and testing on Windows. This page is the readable version of the full per-task log in [`DEVLOG.md`](../DEVLOG.md) (Glass Box track). The same log is exported for itch by `python Scripts/export_devlog.py` → `Dist/devlog_itch.html`.

<!-- TODO(screenshot): title screen / key art -->

## The concept

2–4 players die and become angels. Each round:

1. **Build** — everyone places traps and hidden cameras on the same suburban-backyard obstacle course. Other players' placements stay hidden.
2. **Run** — everyone is reincarnated as a ragdoll mortal and races start to gate, with proximity voice and a handheld camera. Die and you respawn at the start; run out of time and a lightning smite hits you, filmed by a guaranteed "Heaven Cam".
3. **Edit** — 90 seconds to cut the footage of everyone else getting wiped out. Hits are auto-marked; auto-edit saves you if time runs out.
4. **Premiere** — the cuts play on a big screen in a heaven cinema, a narrator introduces director and victim, and everyone rates each cut 1–5 on **Funny** (points to the editor) and **Evil** (points to the trap builder).

The map persists, so traps pile up over three rounds, then a podium.

<!-- TODO(screenshot): build mode with ghost trap + camera preview -->
<!-- TODO(screenshot): a run, ragdoll mid-wipeout -->

## How it fits "Director's Cut"

The theme is the whole loop, not a coat of paint. You are a director twice over: in Build you stage the scene (traps are the stunts, cameras are the shot list), and in Edit you make the actual director's cut from your own angles, with trims, slow-mo, zoom, stickers and captions. Premiere is the screening, and the audience scores you. Because footage is stored as replay data instead of video, every cut is cheap to network and gets slow-mo and zoom for free.

<!-- TODO(screenshot): editor screen timeline -->
<!-- TODO(screenshot): heaven cinema couch + rating reveal -->

## Plan v3 timeline

`plan.md` (v3, plus addendum v3.1 for the unified toon shader) locked the design up front and split the work into small tasks, each runnable by an agent from its task line plus the plan alone. Every task appended a DEVLOG entry: the prompt, what the agent did, what failed, what was fixed by hand.

| Tasks | What |
|---|---|
| 1–3 | Research: UE5 jam assets, party-game UI references, EOS join codes + voice + mic capture |
| 4–10 | Engine-free Core: state machine, placement + budget, deterministic trap timelines, scoring, replay recorder, edit list + auto-edit, ratings |
| 11–13 | UE project + Core module + package smoke build; toon shader; angel character; hats + faces |
| 14–17 | Ragdoll pawn, EOS host/join by code, traps in world + smite, Backyard map + gate |
| 18–22 | Build mode, Run phase, mic capture, editor screen, heaven cinema + ratings |
| 23–26 | Full 3-round match flow, menus + UI skin, narrator/music/SFX, release package + devlog |
| 27–31b | Review fixes, Shipping package (323.6 MiB, under the 1 GB cap), Automation run, Backyard assets, premiere/respawn fix |

<!-- TODO(screenshot): plan.md task list or git log -->

## Core / Unreal split

The rules live in `Core/`: plain C++17, CMake and doctest, no engine. Placement, budget, trap timelines, scoring, replay, edit list, ratings, match flow, menu flow and audio cues are all there, and `make test` runs them (plus the Python script tests) in seconds.

Why: the agents work in Linux sandboxes with no Unreal. Anything in Core can be tested by the agent that wrote it. The Unreal module (`Source/`) only wraps Core: actors, widgets, networking, audio. UE-side code ships with Automation tests (`TTA.*`), but those can only run on Kevin's machine through Session Frontend → Automation, so most UE entries in the log end with "not compiled here, pending Kevin's build".

## Multi-agent workflow

- **Mac side:** sandboxed agents (Waymond via Choomfie, task by task from Discord) wrote Core, the UE C++, scripts, tests and the handoff notes in `plan.md`. Each task lands as one commit on a plan branch.
- **Windows side:** Grok in Cursor on Kevin's Windows PC did what needs the real engine: compiling the UE module with MSVC, fixing compile and link errors, packaging Shipping builds and running Automation, partly on overnight branches (`overnight/cursor`, `mwe4dnfb`).
- **Glue:** `plan.md` is the single source of truth and each task leaves a handoff (done / left / surprises) so the next agent, on either machine, starts from facts instead of memory. `export_devlog.py` fails if a plan task has no DEVLOG entry.

<!-- TODO(screenshot): Cursor on Windows next to a Discord task thread -->

## What went wrong and how it was fixed

### Shipping link error: duplicate Core sources
Core was first wrapped in its own `TTACore` UE module while the game module also pulled in Core sources, so the same symbols were compiled into the link twice and the Windows build failed at link time. Fix (commit "Fix UE compile + link on Windows"): drop the `TTACore` dependency, compile each Core `.cpp` exactly once through one-line wrappers in `Source/TakeTwoAfterlife/Private/CoreLib/Core_*.cpp`, add `Core/include` to the include paths, and turn off unity builds and shared PCHs for the module so files can't be merged or double-included.

### MSVC fixes for Core and UE code
Core was only ever built with GCC/Clang in the sandbox; MSVC under UE's stricter flags was the first real compile. Fixes were small and many: missing includes that other compilers had pulled in transitively (`Engine/StaticMesh.h`, `GameFramework/PlayerState.h`, `TimerManager.h`), a disabled C4458 warning where a local `Owner` shadows `AActor::Owner`, and small edits in several Automation tests. The lesson: compile on the target toolchain early, not on day two.

### Automation crashes and red tests
The first full `TTA.*` run was red. `TTA.UI.MenuFlow` crashed because it ran with no world; ragdoll mesh, build-place meshes, premiere and respawn tests failed. A premiere/respawn fix (task 31b) and review fixes (task 28) were made on the Windows side and still need merging; the Shipping exe has not been launched yet. Status is tracked in `SUBMISSION.md`.

<!-- TODO(screenshot): Session Frontend Automation results -->

### Smaller ones
- Sandbox containers don't keep files between runs, so edits had to be done as one script per run.
- `make test` needed `pip install cmake` in the sandbox.
- Audio (ElevenLabs narrator, WeLoveIndies music) and Kenney assets can't be fetched from the sandbox; Kevin imports them by hand.
- A handoff once claimed `RoundInput::absentOwners` existed when it didn't; the restore task re-added it with tests.

## Credits

Tools and asset attributions: [`CREDITS.md`](../CREDITS.md).
