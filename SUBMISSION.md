# Submission checklist — Epic AI MegaJam 2026

Deadline: Nov 15, 5:59 PM PT. Tracks: Best Student Project + Solo Driver.

## Build (Windows, Kevin's machine)
1. `Scripts\\package_win64.bat "C:\\Program Files\\Epic Games\\UE_5.5" <TeamName>` → `Dist\\<TeamName>_TakeTwoAfterlife.zip`. The script refuses a zip of 1 GB or more.
2. Unzip to a fresh folder, run `TakeTwoAfterlife.exe`: it must open on the Title screen. Host + join by code from a second PC.
3. `python Scripts/export_devlog.py` → `Dist/devlog_itch.html` (fails if any plan task has no DEVLOG entry). Paste into an itch devlog post (Glass Box) and copy its link.

## Upload
- [ ] Win64 zip `<TeamName>_TakeTwoAfterlife.zip`, under 1 GB
- [ ] 30–60 s gameplay video (English, 13+, slapstick only)
- [ ] Epic account email
- [ ] Tools list: the Tools table in CREDITS.md
- [ ] Dev log link (itch devlog)
- [ ] CREDITS.md rows filled (music titles, SFX ids, ElevenLabs attribution)

## Status as of Nov 13
- **Package:** a Shipping zip built at 323.6 MiB (under the 1 GB cap). The packaged `TakeTwoAfterlife.exe` has not been launched yet, so the Title-screen and host/join checks in Build step 2 are still open.
- **Automation suite is red:**
  - `TTA.UI.MenuFlow` crashes: it runs with no world.
  - `TTA.Pawn.Ragdoll`: ragdoll mesh failing.
  - Build-place meshes failing (`TTA.Build.PlaceReplicate`).
  - Premiere (`TTA.Cinema.Sequence`) and respawn failing.
- **Art:** the Backyard map uses placeholder art until `fetch_assets.ps1` runs and the map is rebuilt.
- **Branches:** `overnight/cursor` and `mwe4dnfb` still need merging into this branch.
