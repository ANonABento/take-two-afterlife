# Credits

Every third-party asset, library and tool used in Take Two Afterlife. Human-made assets only.

## Code
| Name | Author | License | Source |
|---|---|---|---|
| doctest 2.4.11 | Viktor Kirilov | MIT | https://github.com/doctest/doctest |

## Art
TODO: confirm the KayKit/Kenney mesh names and pack versions once `fetch_assets.ps1` has run (names below are from memory).
| Asset | Author | License | Source |
|---|---|---|---|
| KayKit Prototype Bits 1.0: `Barrel_A`, `Box_A`, `Box_B`, `Coin_A`, `Pallet_Small`, `Primitive_Floor`, `table_medium` (Backyard map; materials replaced with M_TTA_Toon) | Kay Lousberg | CC0 1.0 | https://github.com/KayKit-Game-Assets/KayKit-Prototype-Bits-1.0 |
| City Kit (Suburban): `building-type-a`, `building-type-b`, `building-type-c`, `building-type-d`, `building-type-e`, `building-type-f`, `fence`, `planter`, `tree-large`, `tree-small` (Backyard map; materials replaced with M_TTA_Toon) | Kenney | CC0 1.0 | https://kenney.nl/assets/city-kit-suburban |
| Nature Kit: `flower_redA`, `flower_yellowA`, `plant_bushLarge`, `rock_smallA` (Backyard map; materials replaced with M_TTA_Toon) | Kenney | CC0 1.0 | https://kenney.nl/assets/nature-kit |
| Backyard props: `SM_Backyard_Gnome`, `SM_Backyard_Grill`, `SM_Backyard_KiddiePool`, `SM_Backyard_PicnicTable`, `SM_Backyard_Shed`, `SM_Backyard_Trampoline` (Backyard map) | Take Two: Afterlife team, scripted in Blender | own work | models/build_backyard_props.py |
| Trapdoor `trapdoor_quaternius.glb` (`SM_Trap_Trapdoor`, recoloured from palette.md) | Quaternius | CC0 1.0 | https://poly.pizza/m/Ph6MpmwySS |
| Banana `banana_kenney.glb`, Food Kit (`SM_Trap_Banana`, recoloured from palette.md) | Kenney | CC0 1.0 | https://poly.pizza/m/5Z07I9KMob |
| Doublesided Hammer `hammer_quaternius.glb` (`SM_Trap_Hammer`, recoloured from palette.md) | Quaternius | CC0 1.0 | https://poly.pizza/m/UIXvQ73DS1 |
| Saw Blade `sawblade_quaternius.glb` (`SM_Trap_Spinner`, recoloured from palette.md) | Quaternius | CC0 1.0 | https://poly.pizza/m/HLOlecPOVD |
| Traps: `SM_Trap_Lawnmower`, `SM_Trap_PusherWall`, `SM_Trap_Cannon` and primitive fallbacks for the four above | Take Two: Afterlife team, scripted in Blender | own work | models/build_traps_cams.py |
| Course kit: `SM_Course_GateFinish` (Backyard finish arch) | Take Two: Afterlife team, scripted in Blender | own work | models/build_course_kit.py |

## Audio
TODO: fill in track titles/artists, Freesound ids/authors and the ElevenLabs voice/model once the audio files are imported.
| Asset | Author | License | Source |
|---|---|---|---|
| Narrator voice, 60 lines `VO_*` (script exported by `Core/tools/vo_script.cpp` → `build/tta_vo_script`; voice/model: TODO) | ElevenLabs text-to-speech, script by Kevin | ElevenLabs Free tier: attribution required ("Voice generated with ElevenLabs, elevenlabs.io") | https://elevenlabs.io |
| SFX pack `Audio/SFX/*.wav`: trap_place, countdown_beep, clock_tick, bonk, thunder, gate_chime, projector_start, rating_ding, applause, fanfare (synthesised by `Scripts/make_sfx.py`, numpy + wave) | generated in-house | CC0 1.0 | `Scripts/make_sfx.py` |
| Music `MUS_Lobby`, `MUS_Build`, `MUS_Run`, `MUS_Edit`, `MUS_Premiere`, `MUS_Podium` (one track per phase; fill in track titles/artists) | WeLoveIndies artists | WeLoveIndies licence (check terms on download) | https://www.weloveindies.com |
| SFX `SFX_TrapPlace`, `SFX_CountdownBeep`, `SFX_ClockTick`, `SFX_ImpactBonk`, `SFX_Thunder`, `SFX_GateChime`, `SFX_ProjectorStart`, `SFX_RatingDing`, `SFX_Applause`, `SFX_Fanfare` (fill in each Freesound sound id/author; CC0 preferred, CC-BY credited here) | Freesound / Kenney audio packs | CC0 1.0 / CC-BY 4.0 per sound | https://freesound.org, https://kenney.nl/assets/category:Audio |

## Tools
| Tool | Use |
|---|---|
| Unreal Engine 5.5 (C++, UMG, Python editor scripting, Automation) | game engine, packaging, tests |
| Epic Online Services (lobbies, P2P relay, voice) | join codes, networking, proximity voice |
| CMake + doctest | engine-free `Core/` build and unit tests |
| Python 3 | editor scripts, zip/devlog tools, tests |
| Blender | angel, hats and faces (scripted builds in `models/`) |
| ElevenLabs | narrator text-to-speech |
| OBS Studio | gameplay video capture |
| Choomfie / Waymond (Claude) coding agents | code, scripts and research per DEVLOG.md |
| Tripo | AI-generated 3D models (TODO: list which assets) |
| Image generator (via Ambition/Waymond) | AI-generated UI art in `Art/UI/` (`T_UI_*.png`): generated on magenta, chroma-keyed and cropped in Python |
| Git / GitHub | version control |
